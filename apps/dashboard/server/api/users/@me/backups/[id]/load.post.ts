import { createReadStream, promises as fsp } from 'node:fs';

import { filesize } from 'filesize';
import { nanoid } from 'nanoid';
import { extname, join } from 'pathe';
import chain from 'stream-chain';
import { parser } from 'stream-json';
import { streamArray } from 'stream-json/streamers/stream-array.js';
import { extract } from 'tar';
import { z } from 'zod';

import { insert, removeMultiple } from '@orama/orama';

import { AIJobType, BackupRestoreState } from '#shared/prisma/enums';

const validationSchema = z
    .object({
        verificationData: z.any().optional(),
    })
    .optional();

const BACKUP_FIELDS = {
    folder: ['name', 'public', 'createdAt'],
    note: ['title', 'content', 'public', 'createdAt'],
    file: [
        'mimeType',
        'maxViews',
        'expiresAt',
        'password',
        'createdAt',
        'caption',
        'piiDetected',
        'piiReasons',
        'ocrText',
        'embedding',
        'textEmbedding',
    ],
    view: ['ip', 'createdAt'],
    user: [],
};

const pickBackupFields = (database: keyof typeof BACKUP_FIELDS, value: Record<string, any>) =>
    Object.fromEntries(
        BACKUP_FIELDS[database].filter((key) => key in value).map((key) => [key, value[key]]),
    ) as Record<string, any>;

export default defineEventHandler(async (event) => {
    userOnly(event);
    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw createError({
            status: 400,
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    const backup = await findBackup(currentUser.id, getRouterParam(event, 'id')!);

    await verifySession(event, body.data?.verificationData);

    const user = await prisma.user.findUnique({
        where: { id: currentUser.id },
        select: { backupRestoreState: true },
    });

    if (user?.backupRestoreState) {
        throw createError({
            status: 400,
            message: 'A backup restoration is already in progress',
        });
    }

    const updateState = async (state: BackupRestoreState | null) => {
        await prisma.user.update({
            where: { id: currentUser.id },
            data: {
                backupRestoreState: state,
                ...(state === BackupRestoreState.Extracting || !state
                    ? { backupRestoreStartedAt: state ? new Date() : null }
                    : {}),
            },
        });
        sendToUser(currentUser.id, 'currentUser:update', { backupRestoreState: state });
    };

    const tempPath = join(dataDirectory, 'temp', `restore-${backup.id}`);

    event.waitUntil(
        (async () => {
            try {
                await fsp.mkdir(tempPath, { recursive: true });

                await updateState(BackupRestoreState.Extracting);
                await withLocalFile(backup.storageId, backupKey(backup), (backupPath) =>
                    extract({
                        file: backupPath,
                        cwd: tempPath,
                        filter: (_, entry) =>
                            'type' in entry && ['File', 'Directory'].includes(entry.type),
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
                            OR: [
                                { ownerId: currentUser.id },
                                { file: { authorId: currentUser.id } },
                            ],
                        },
                    }),
                    prisma.folder.deleteMany({
                        where: { authorId: currentUser.id, inbox: null },
                    }),
                    prisma.note.deleteMany({ where: { authorId: currentUser.id } }),
                    prisma.file.deleteMany({ where: { authorId: currentUser.id } }),
                ]);

                sendToUser(currentUser.id, 'deleteAll', null);

                await updateState(BackupRestoreState.RestoringData);

                const databases = ['folder', 'note', 'file', 'user', 'view'] as const;
                const remappedKeys = new Map<string, string>();
                const uploadSizes = new Map<string, number>();
                const uploadPaths = new Map<string, string>();
                const storageId = await getActiveStorageId();
                const storage = await getStorage(storageId);

                for (const { name, size } of backupUploads) {
                    const remappedName = (await prisma.file.findUnique({
                        where: { fileName: name },
                    }))
                        ? `${nanoid(8)}${extname(name)}`
                        : name;

                    remappedKeys.set(name, remappedName);
                    uploadSizes.set(remappedName, size);
                    uploadPaths.set(remappedName, join(backupUploadsPath, name));
                }

                for (const database of databases) {
                    const databasePath = join(tempPath, 'database', `${database}.json`);

                    if (database === 'user') {
                        try {
                            const { avatar, domains, embed } = JSON.parse(
                                await fsp.readFile(databasePath, 'utf-8'),
                            );

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
                        } catch {
                            //
                        }
                    } else {
                        await new Promise<void>((resolve) => {
                            const pipeline = chain([
                                createReadStream(databasePath),
                                parser(),
                                streamArray(),
                            ]);

                            let processing = Promise.resolve();

                            pipeline.on('data', async ({ value }: any) => {
                                processing = processing.then(async () => {
                                    const data = pickBackupFields(database, value);

                                    switch (database) {
                                        case 'file': {
                                            const fileName = remappedKeys.get(value.fileName);
                                            if (!fileName) return;

                                            data.fileName = fileName;
                                            data.size = BigInt(uploadSizes.get(fileName)!);
                                            data.folderId =
                                                remappedKeys.get(value.folderId) ?? null;
                                            data.authorId = currentUser.id;
                                            data.storageId = storageId;
                                            break;
                                        }
                                        case 'view':
                                            data.fileId = remappedKeys.get(value.fileId) ?? null;
                                            data.ownerId = currentUser.id;
                                            break;
                                        default:
                                            data.authorId = currentUser.id;
                                    }

                                    try {
                                        const created =
                                            database === 'folder' && value.isInbox
                                                ? await prisma.folder.findFirst({
                                                      where: { inbox: { userId: currentUser.id } },
                                                  })
                                                : await (prisma as any)[database].create({ data });
                                        remappedKeys.set(value.id, created.id);

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
                                                        formatted: filesize(
                                                            created.size.toString(),
                                                        ),
                                                    };

                                                    const uploadPath = uploadPaths.get(
                                                        created.fileName,
                                                    )!;

                                                    if (isAnalyzable(created.mimeType))
                                                        created.metadata = await processMedia(
                                                            created,
                                                            uploadPath,
                                                        );

                                                    await storage.put(
                                                        uploadKey(created.fileName),
                                                        uploadPath,
                                                    );

                                                    created.thumbnailUrl = getThumbnailUrl(
                                                        event,
                                                        currentUser.domains,
                                                        created,
                                                    );

                                                    const aiEnabled =
                                                        currentUser.aiSettings?.enabled ?? true;

                                                    if (aiEnabled) {
                                                        if (
                                                            ai.IMAGE_EMBEDDING_SUPPORTED_EXTENSIONS.includes(
                                                                extname(created.fileName),
                                                            )
                                                        ) {
                                                            event.waitUntil(
                                                                Promise.all([
                                                                    enqueueAIJob({
                                                                        userId: currentUser.id,
                                                                        fileId: created.id,
                                                                        type: AIJobType.GenerateClipEmbedding,
                                                                    }),
                                                                    enqueueAIJob({
                                                                        userId: currentUser.id,
                                                                        fileId: created.id,
                                                                        type: AIJobType.GenerateOcrText,
                                                                    }),
                                                                    enqueueAIJob({
                                                                        userId: currentUser.id,
                                                                        fileId: created.id,
                                                                        type: AIJobType.GenerateImageCaption,
                                                                    }),
                                                                    enqueueAIJob({
                                                                        userId: currentUser.id,
                                                                        fileId: created.id,
                                                                        type: AIJobType.DetectPII,
                                                                    }),
                                                                ]),
                                                            );
                                                        } else if (
                                                            ai.VIDEO_EMBEDDING_SUPPORTED_EXTENSIONS.includes(
                                                                extname(created.fileName),
                                                            )
                                                        ) {
                                                            event.waitUntil(
                                                                Promise.all([
                                                                    enqueueAIJob({
                                                                        userId: currentUser.id,
                                                                        fileId: created.id,
                                                                        type: AIJobType.GenerateVideoEmbedding,
                                                                    }),
                                                                ]),
                                                            );
                                                        } else if (
                                                            ai.TEXT_EMBEDDING_SUPPORTED_EXTENSIONS.includes(
                                                                extname(created.fileName),
                                                            )
                                                        ) {
                                                            event.waitUntil(
                                                                Promise.all([
                                                                    enqueueAIJob({
                                                                        userId: currentUser.id,
                                                                        fileId: created.id,
                                                                        type: AIJobType.GenerateTextEmbedding,
                                                                    }),
                                                                    enqueueAIJob({
                                                                        userId: currentUser.id,
                                                                        fileId: created.id,
                                                                        type: AIJobType.DetectPII,
                                                                    }),
                                                                ]),
                                                            );
                                                        }
                                                    }

                                                    await insert(fileSearchDb, {
                                                        id: created.id,
                                                        authorId: created.authorId,
                                                        fileName: created.fileName,
                                                        mimeType: created.mimeType,
                                                    });
                                                }
                                                break;
                                            case 'folder':
                                                created.publicUrl = created.public
                                                    ? buildPublicUrl(
                                                          event,
                                                          currentUser.domains,
                                                          `/folder/${created.id}`,
                                                      )
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

                                        sendToUser(currentUser.id, `${database}:create`, created);
                                    } catch {
                                        //
                                    }
                                });
                            });

                            pipeline.on('end', async () => {
                                await processing;
                                resolve();
                            });
                        });
                    }
                }

                await fsp.rm(tempPath, { recursive: true, force: true });
                await createLog(event, {
                    action: 'Load Backup',
                    message: `Loaded backup ${backup.id}`,
                });
                await updateState(null);
            } catch {
                await fsp.rm(tempPath, { recursive: true, force: true }).catch(() => null);
                await createLog(event, {
                    action: 'Load Backup Failed',
                    message: `Failed to load backup ${backup.id}`,
                });
                await updateState(null);
            }
        })(),
    );
});
