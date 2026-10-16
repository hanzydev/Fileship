import { createReadStream, existsSync, promises as fsp } from 'node:fs';

import { nanoid } from 'nanoid';
import { join } from 'pathe';
import chain from 'stream-chain';
import { parser } from 'stream-json';
import { streamArray } from 'stream-json/streamers/stream-array.js';
import { extract } from 'tar';
import { z } from 'zod';

import { insert, removeMultiple } from '@orama/orama';

import { BackupRestoreState } from '#shared/prisma/enums';

const validationSchema = z
    .object({
        verificationData: z.any().optional(),
    })
    .optional();

const BACKUP_FIELDS = {
    folder: ['name', 'public', 'createdAt'],
    note: ['title', 'content', 'public', 'createdAt'],
    file: [
        'name',
        'mimeType',
        'maxViews',
        'expiresAt',
        'password',
        'createdAt',
        'piiDetected',
        'piiReasons',
        'piiOverride',
    ],
    view: ['ip', 'createdAt'],
    link: ['code', 'url', 'password', 'maxClicks', 'clicks', 'expiresAt', 'createdAt'],
    user: [],
};

const BACKUP_ROW_SCHEMAS = {
    folder: z.object({ name: z.string().min(1).max(32), public: z.boolean() }),
    note: z.object({
        title: z.string().trim().min(3).max(48),
        content: z.string().min(1).max(50_000),
        public: z.boolean(),
    }),
    file: z.object({
        piiDetected: z.boolean().optional(),
        piiReasons: z
            .array(z.string().refine((reason) => Object.hasOwn(PII_REASONS, reason)))
            .optional(),
    }),
    link: z.object({ url: linkUrlSchema }),
};

const DATABASES = ['folder', 'note', 'link', 'file', 'user', 'view'] as const;

type RowDatabase = Exclude<(typeof DATABASES)[number], 'user'>;

const pickBackupFields = (database: keyof typeof BACKUP_FIELDS, value: Record<string, any>) =>
    Object.fromEntries(
        BACKUP_FIELDS[database].filter((key) => key in value).map((key) => [key, value[key]]),
    ) as Record<string, any>;

export default defineEventHandler(async (event) => {
    userOnly(event);
    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw invalidBodyError(body.error);
    }

    const backup = await findBackup(currentUser.id, getRouterParam(event, 'id')!);

    const canRestore = {
        folder: canUploadFiles(event),
        file: canUploadFiles(event),
        view: canUploadFiles(event),
        note: canTakeNotes(event),
        link: canShortenLinks(event),
        user: true,
    };

    await verifySession(event, body.data?.verificationData);

    const { count } = await prisma.user.updateMany({
        where: { id: currentUser.id, backupRestoreState: null },
        data: {
            backupRestoreState: BackupRestoreState.Extracting,
            backupRestoreStartedAt: new Date(),
        },
    });

    if (!count) {
        throw apiError(400, 'errorsUsers.restoreInProgress');
    }

    sendToUser(currentUser.id, 'currentUser:update', {
        backupRestoreState: BackupRestoreState.Extracting,
    });

    const updateState = async (state: BackupRestoreState | null) => {
        await prisma.user.update({
            where: { id: currentUser.id },
            data: {
                backupRestoreState: state,
                ...(!state && { backupRestoreStartedAt: null }),
            },
        });
        sendToUser(currentUser.id, 'currentUser:update', { backupRestoreState: state });
    };

    const restoreUser = async (databasePath: string) => {
        const { avatar, domains, embed } = JSON.parse(await fsp.readFile(databasePath, 'utf-8'));

        await prisma.user.update({
            where: { id: currentUser.id },
            data: { avatar, domains, embed },
        });

        sendToUser(currentUser.id, 'currentUser:domainsUpdate', domains);
        sendToUser(currentUser.id, 'currentUser:embedUpdate', embed);

        await sendByFilter(isAdmin, 'user:update', {
            id: currentUser.id,
            avatar,
        });

        sendToUser(currentUser.id, 'currentUser:update', {
            avatar,
        });
    };

    const tempPath = join(dataDirectory, 'temp', `restore-${backup.id}`);

    const restore = async () => {
        await fsp.mkdir(tempPath, { recursive: true });

        await withLocalFile(backup.storageId, backupKey(backup), (backupPath) =>
            extract({
                file: backupPath,
                cwd: tempPath,
                filter: (_, entry) => 'type' in entry && ['File', 'Directory'].includes(entry.type),
            }),
        );

        const backupUploadsPath = join(tempPath, 'uploads');
        const backupUploads: { name: string; size: number }[] = [];

        for (const name of await fsp.readdir(backupUploadsPath).catch(() => [])) {
            const stat = await fsp.lstat(join(backupUploadsPath, name));
            if (stat.isFile()) backupUploads.push({ name, size: stat.size });
        }

        const backupSize = backupUploads.reduce((total, upload) => total + upload.size, 0);

        if (
            !isAdmin(currentUser) &&
            currentUser.limits.usableSpace > -1 &&
            backupSize > currentUser.limits.usableSpace * 1024 * 1024
        ) {
            throw new Error('Backup exceeds the storage limit');
        }

        await updateState(BackupRestoreState.DeletingPreviousData);

        const [userFiles, userFolders, userNotes] = await prisma.$transaction([
            prisma.file.findMany({
                where: {
                    authorId: currentUser.id,
                },
                select: {
                    id: true,
                    fileName: true,
                    storageId: true,
                },
            }),
            prisma.folder.findMany({
                where: {
                    authorId: currentUser.id,
                    inbox: null,
                },
                select: {
                    id: true,
                },
            }),
            prisma.note.findMany({
                where: {
                    authorId: currentUser.id,
                },
                select: {
                    id: true,
                },
            }),
        ]);

        await removeFileData(userFiles);

        await removeMultiple(
            folderSearchDb,
            userFolders.map((f) => f.id),
        );

        await removeMultiple(
            noteSearchDb,
            userNotes.map((n) => n.id),
        );

        await prisma.$transaction([
            prisma.view.deleteMany({
                where: {
                    OR: [{ ownerId: currentUser.id }, { file: { authorId: currentUser.id } }],
                },
            }),
            prisma.folder.deleteMany({
                where: { authorId: currentUser.id, inbox: null },
            }),
            prisma.note.deleteMany({ where: { authorId: currentUser.id } }),
            prisma.link.deleteMany({ where: { authorId: currentUser.id } }),
            prisma.file.deleteMany({ where: { authorId: currentUser.id } }),
        ]);

        sendToUser(currentUser.id, 'deleteAll', null);

        await updateState(BackupRestoreState.RestoringData);

        const fileNames = new Map<string, string>();
        const uploads = new Map<string, { path: string; size: number }>();
        const restoredIds = { folder: new Map<string, string>(), file: new Map<string, string>() };
        const storageId = await getActiveStorageId();
        const storage = await getStorage(storageId);

        for (const { name, size } of backupUploads) {
            const fileName =
                isValidFileName(name) &&
                !(await prisma.file.findUnique({ where: { fileName: name } }))
                    ? name
                    : `${nanoid(8)}${safeExtension(name)}`;

            fileNames.set(name, fileName);
            uploads.set(fileName, { path: join(backupUploadsPath, name), size });
        }

        const restoreRow = async (database: RowDatabase, value: any) => {
            if (!value || typeof value !== 'object') return;

            const schema = BACKUP_ROW_SCHEMAS[database as keyof typeof BACKUP_ROW_SCHEMAS];
            if (schema && !schema.safeParse(value).success) return;

            const data = pickBackupFields(database, value);

            switch (database) {
                case 'file': {
                    const fileName =
                        typeof value.fileName === 'string' ? fileNames.get(value.fileName) : null;
                    if (!fileName) return;

                    data.fileName = fileName;
                    data.name = toDisplayName(
                        typeof value.name === 'string' ? value.name : '',
                        fileName,
                    );
                    data.size = BigInt(uploads.get(fileName)!.size);
                    data.folderId = restoredIds.folder.get(value.folderId) ?? null;
                    data.authorId = currentUser.id;
                    data.storageId = storageId;
                    data.piiOverride =
                        typeof value.piiOverride === 'boolean' ? value.piiOverride : null;
                    data.piiDetected = data.piiOverride ?? value.piiDetected === true;
                    data.piiReasons = data.piiDetected ? (value.piiReasons ?? []) : [];
                    break;
                }
                case 'view':
                    data.fileId = restoredIds.file.get(value.fileId) ?? null;
                    data.ownerId = currentUser.id;
                    break;
                case 'link':
                    data.authorId = currentUser.id;
                    if (
                        !linkCodeSchema.safeParse(data.code).success ||
                        (await prisma.link.findUnique({
                            where: { code: data.code },
                        }))
                    )
                        data.code = await createLinkCode();
                    break;
                default:
                    data.authorId = currentUser.id;
            }

            const created =
                database === 'folder' && value.isInbox
                    ? await prisma.folder.findFirst({
                          where: { inbox: { userId: currentUser.id } },
                      })
                    : await (prisma as any)[database].create({ data });
            if (!created) return;

            if ((database === 'folder' || database === 'file') && typeof value.id === 'string')
                restoredIds[database].set(value.id, created.id);

            switch (database) {
                case 'file':
                    {
                        created.directUrl = buildPublicUrl(
                            event,
                            currentUser.domains,
                            `/u/${created.fileName}`,
                        );
                        created.embedUrl = buildPublicUrl(
                            event,
                            currentUser.domains,
                            `/view/${created.fileName}`,
                        );
                        created.size = {
                            raw: created.size.toString(),
                            formatted: formatSize(created.size.toString()),
                        };

                        const uploadPath = uploads.get(created.fileName)!.path;

                        try {
                            if (isAnalyzable(created.mimeType))
                                created.metadata = await processMedia(created, uploadPath);

                            await storage.put(uploadKey(created.fileName), uploadPath);
                        } catch (error) {
                            await prisma.file.delete({ where: { id: created.id } });
                            await removeFileData([created]);
                            throw error;
                        }

                        created.thumbnailUrl = getThumbnailUrl(event, currentUser.domains, created);

                        await indexCreatedFile(event, currentUser, created);
                    }
                    break;
                case 'folder':
                    created.publicUrl = created.public
                        ? buildPublicUrl(event, currentUser.domains, `/folder/${created.id}`)
                        : undefined;
                    created.files = [];

                    await insert(folderSearchDb, {
                        id: created.id,
                        authorId: created.authorId,
                        name: created.name,
                    });
                    break;
                case 'note':
                    await insert(noteSearchDb, {
                        id: created.id,
                        authorId: created.authorId,
                        title: created.title,
                    });
                    break;
            }

            if (database === 'file') {
                created.views = { today: 0, total: 0 };
            }

            if (database !== 'view') sendToUser(currentUser.id, `${database}:create`, created);
        };

        for (const database of DATABASES) {
            const databasePath = join(tempPath, 'database', `${database}.json`);

            if (!existsSync(databasePath) || !canRestore[database]) continue;

            if (database === 'user') {
                await restoreUser(databasePath).catch(() => null);
                continue;
            }

            const rows = chain([createReadStream(databasePath), parser(), streamArray()]);

            try {
                for await (const { value } of rows) {
                    await restoreRow(database, value).catch(() => null);
                }
            } catch {
                //
            } finally {
                rows.destroy();
            }
        }
    };

    event.waitUntil(
        (async () => {
            try {
                await restore();
                await createLog(event, {
                    action: 'Load Backup',
                    message: `Loaded backup ${backup.id}`,
                });
            } catch {
                await createLog(event, {
                    action: 'Load Backup Failed',
                    message: `Failed to load backup ${backup.id}`,
                }).catch(() => null);
            } finally {
                await fsp.rm(tempPath, { recursive: true, force: true }).catch(() => null);
                await updateState(null).catch(() => null);
            }
        })(),
    );
});
