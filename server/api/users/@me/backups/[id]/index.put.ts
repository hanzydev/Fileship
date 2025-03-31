import { createReadStream, existsSync, promises as fsp } from 'node:fs';
import { nextTick } from 'node:process';

import { filesize } from 'filesize';
import fluentFfmpeg from 'fluent-ffmpeg';
import { nanoid } from 'nanoid';
import { extname, join } from 'pathe';
import Chain from 'stream-chain';
import parser from 'stream-json';
import StreamArray from 'stream-json/streamers/StreamArray.js';
import { extract } from 'tar';
import { z } from 'zod';

import ffmpeg from '@ffmpeg-installer/ffmpeg';
import { BackupRestoreState } from '@prisma/client';

fluentFfmpeg.setFfmpegPath(ffmpeg.path);

const validationSchema = z
    .object({
        verificationData: z.any().optional(),
    })
    .optional();

export default defineEventHandler(async (event) => {
    userOnly(event);
    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw createError({
            statusCode: 400,
            statusMessage: 'Bad Request',
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    const backupId = getRouterParam(event, 'id');
    const backupPath = join(dataDirectory, 'backups', currentUser.id, `${backupId}.tgz`);

    if (!existsSync(backupPath)) {
        throw createError({
            statusCode: 404,
            statusMessage: 'Not Found',
            message: 'Backup not found',
        });
    }

    await verifySession(event, body.data?.verificationData);

    const updateState = async (state: BackupRestoreState | null) => {
        await prisma.user.update({
            where: { id: currentUser.id },
            data: { backupRestoreState: state },
        });
        sendToUser(currentUser.id, 'update:currentUser', { backupRestoreState: state });
    };

    await updateState(BackupRestoreState.DeletingPreviousData);

    const userFiles = await prisma.file.findMany({
        where: { authorId: currentUser.id },
        select: { id: true, fileName: true },
    });

    const uploadsPath = join(dataDirectory, 'uploads');
    const thumbnailsPath = join(dataDirectory, 'thumbnails');

    await Promise.all(
        userFiles.map(({ fileName }) =>
            fsp.rm(join(uploadsPath, fileName), { force: true }).catch(() => null),
        ),
    );

    await Promise.all(
        userFiles.map(({ id }) =>
            fsp.rm(join(thumbnailsPath, `${id}.jpeg`), { force: true }).catch(() => null),
        ),
    );

    await prisma.$transaction([
        prisma.view.deleteMany({
            where: {
                OR: [
                    { file: { authorId: currentUser.id } },
                    { code: { authorId: currentUser.id } },
                    { url: { authorId: currentUser.id } },
                ],
            },
        }),
        prisma.folder.deleteMany({ where: { authorId: currentUser.id } }),
        prisma.note.deleteMany({ where: { authorId: currentUser.id } }),
        prisma.code.deleteMany({ where: { authorId: currentUser.id } }),
        prisma.url.deleteMany({ where: { authorId: currentUser.id } }),
        prisma.file.deleteMany({ where: { authorId: currentUser.id } }),
    ]);

    sendToUser(currentUser.id, 'delete:all', null);

    const tempPath = join(dataDirectory, 'temp', backupId!);
    await fsp.mkdir(tempPath);

    await updateState(BackupRestoreState.Extracting);
    await extract({ file: backupPath, cwd: tempPath });
    await updateState(BackupRestoreState.RestoringData);

    const databases = ['folder', 'note', 'code', 'url', 'file', 'user', 'view'];
    const backupUploadsPath = join(tempPath, 'uploads');
    const backupUploads = await fsp.readdir(backupUploadsPath);

    const remappedKeys = new Map<string, string>();

    for (const backupUpload of backupUploads) {
        const remappedName = existsSync(join(uploadsPath, backupUpload))
            ? `${nanoid(8)}${extname(backupUpload)}`
            : backupUpload;

        remappedKeys.set(backupUpload, remappedName);

        await fsp
            .rename(join(backupUploadsPath, backupUpload), join(uploadsPath, remappedName))
            .catch(() => null);
    }

    for (const database of databases) {
        const databasePath = join(tempPath, 'database', `${database}.json`);

        if (database === 'user') {
            try {
                const userData = JSON.parse(await fsp.readFile(databasePath, 'utf-8'));
                await prisma.user.update({ where: { id: currentUser.id }, data: userData });

                sendToUser(currentUser.id, 'update:domains', userData.domains);
                sendToUser(currentUser.id, 'update:embed', userData.embed);

                await sendByFilter(isAdmin, 'update:user', {
                    id: currentUser.id,
                    avatar: userData.avatar,
                });

                sendToUser(currentUser.id, 'update:currentUser', { avatar: userData.avatar });
            } catch {
                //
            }
        } else {
            await new Promise<void>((resolve) => {
                const chain = Chain([createReadStream(databasePath), parser(), new StreamArray()]);

                let processing = Promise.resolve();

                chain.on('data', async ({ value }) => {
                    processing = processing.then(async () => {
                        if (value.authorId) value.authorId = currentUser.id;

                        switch (database) {
                            case 'file':
                                value.fileName = remappedKeys.get(value.fileName);

                                if (value.folderId) {
                                    value.folderId = remappedKeys.get(value.folderId);
                                }
                                break;
                            case 'view': {
                                const fileId = remappedKeys.get(value.fileId);
                                const codeId = remappedKeys.get(value.codeId);
                                const urlId = remappedKeys.get(value.urlId);

                                if (fileId) value.file = { connect: { id: fileId } };
                                else if (codeId) value.code = { connect: { id: codeId } };
                                else if (urlId) value.url = { connect: { id: urlId } };

                                value.fileId = undefined;
                                value.codeId = undefined;
                                value.urlId = undefined;

                                break;
                            }
                            case 'url':
                                if (
                                    await prisma.url.findUnique({ where: { vanity: value.vanity } })
                                )
                                    value.vanity = nanoid(8);
                                break;
                        }

                        try {
                            const created = await (prisma as any)[database].create({
                                data: { ...value, id: undefined },
                            });
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
                                            formatted: filesize(created.size.toString()),
                                        };

                                        if (created.mimeType.startsWith('video/')) {
                                            const filePath = join(uploadsPath, created.fileName);

                                            const thumbnailPath = join(
                                                thumbnailsPath,
                                                `${created.id}.jpeg`,
                                            );

                                            await new Promise<void>((resolve) => {
                                                fluentFfmpeg(filePath)
                                                    .videoFilters('thumbnail')
                                                    .frames(1)
                                                    .format('mjpeg')
                                                    .output(thumbnailPath)
                                                    .on('end', () => {
                                                        created.thumbnailUrl = buildPublicUrl(
                                                            event,
                                                            currentUser.domains,
                                                            `/u/${created.fileName}/thumbnail`,
                                                        );
                                                        resolve();
                                                    })
                                                    .on('error', () => resolve())
                                                    .run();
                                            });
                                        }
                                    }
                                    break;
                                case 'code':
                                    created.url = buildPublicUrl(
                                        event,
                                        currentUser.domains,
                                        `/code/${created.id}`,
                                    );
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
                                    break;
                                case 'url':
                                    created.url = buildPublicUrl(
                                        event,
                                        currentUser.domains,
                                        `/link/${created.vanity}`,
                                    );
                                    break;
                            }

                            if (['file', 'code', 'url'].includes(database)) {
                                created.views = { today: 0, total: 0 };
                            }

                            sendToUser(currentUser.id, `create:${database}`, created);

                            if (created.folderId) {
                                nextTick(() =>
                                    sendToUser(currentUser.id, 'folder:file:add', {
                                        folderId: created.folderId,
                                        fileId: created.id,
                                    }),
                                );
                            }
                        } catch {
                            //
                        }
                    });
                });

                chain.on('end', async () => {
                    await processing;
                    resolve();
                });
            });
        }
    }

    await fsp.rm(tempPath, { recursive: true });
    await createLog(event, { action: 'Load Backup', message: `Loaded backup ${backupId}` });
    await updateState(null);
});
