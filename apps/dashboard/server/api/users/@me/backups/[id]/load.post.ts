import { createReadStream, existsSync, promises as fsp } from 'node:fs';

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
import { insert, removeMultiple } from '@orama/orama';

import { BackupRestoreState } from '#shared/prisma/enums';
import { AIJobType } from '~~/generated/prisma/enums';

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
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    const backupId = getRouterParam(event, 'id');
    const backupPath = join(dataDirectory, 'backups', currentUser.id, `${backupId}.tgz`);

    if (!existsSync(backupPath)) {
        throw createError({
            statusCode: 404,
            message: 'Backup not found',
        });
    }

    await verifySession(event, body.data?.verificationData);

    const user = await prisma.user.findUnique({
        where: { id: currentUser.id },
        select: { backupRestoreState: true },
    });

    if (user?.backupRestoreState) {
        throw createError({
            statusCode: 400,
            message: 'A backup restoration is already in progress',
        });
    }

    const updateState = async (state: BackupRestoreState | null) => {
        await prisma.user.update({
            where: { id: currentUser.id },
            data: { backupRestoreState: state },
        });
        sendToUser(currentUser.id, 'currentUser:update', { backupRestoreState: state });
    };

    event.waitUntil(
        (async () => {
            try {
                await updateState(BackupRestoreState.DeletingPreviousData);

                const [userFiles, userFolders, userNotes] = await prisma.$transaction([
                    prisma.file.findMany({
                        where: {
                            authorId: currentUser.id,
                        },
                        select: {
                            id: true,
                            fileName: true,
                        },
                    }),
                    prisma.folder.findMany({
                        where: {
                            authorId: currentUser.id,
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

                const uploadsPath = join(dataDirectory, 'uploads');
                const thumbnailsPath = join(dataDirectory, 'thumbnails');

                await Promise.all(
                    userFiles.map(({ fileName }) =>
                        fsp.rm(join(uploadsPath, fileName), { force: true }).catch(() => null),
                    ),
                );

                await Promise.all(
                    userFiles.map(({ id }) =>
                        fsp
                            .rm(join(thumbnailsPath, `${id}.jpeg`), { force: true })
                            .catch(() => null),
                    ),
                );

                await removeMultiple(
                    fileSearchDb,
                    userFiles.map((f) => f.id),
                );

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
                            file: { authorId: currentUser.id },
                        },
                    }),
                    prisma.folder.deleteMany({ where: { authorId: currentUser.id } }),
                    prisma.note.deleteMany({ where: { authorId: currentUser.id } }),
                    prisma.file.deleteMany({ where: { authorId: currentUser.id } }),
                ]);

                sendToUser(currentUser.id, 'deleteAll', null);

                const tempPath = join(dataDirectory, 'temp', backupId!);
                await fsp.mkdir(tempPath);

                await updateState(BackupRestoreState.Extracting);
                await extract({ file: backupPath, cwd: tempPath });
                await updateState(BackupRestoreState.RestoringData);

                const databases = ['folder', 'note', 'file', 'user', 'view'];
                const backupUploadsPath = join(tempPath, 'uploads');
                const backupUploads = await fsp.readdir(backupUploadsPath);

                const remappedKeys = new Map<string, string>();

                for (const backupUpload of backupUploads) {
                    const remappedName = existsSync(join(uploadsPath, backupUpload))
                        ? `${nanoid(8)}${extname(backupUpload)}`
                        : backupUpload;

                    remappedKeys.set(backupUpload, remappedName);

                    await moveFileRobust(
                        join(backupUploadsPath, backupUpload),
                        join(uploadsPath, remappedName),
                    ).catch(() => null);
                }

                for (const database of databases) {
                    const databasePath = join(tempPath, 'database', `${database}.json`);

                    if (database === 'user') {
                        try {
                            const userData = JSON.parse(await fsp.readFile(databasePath, 'utf-8'));
                            await prisma.user.update({
                                where: { id: currentUser.id },
                                data: userData,
                            });

                            sendToUser(
                                currentUser.id,
                                'currentUser:domainsUpdate',
                                userData.domains,
                            );
                            sendToUser(currentUser.id, 'currentUser:embedUpdate', userData.embed);

                            await sendByFilter(isAdmin, 'user:update', {
                                id: currentUser.id,
                                avatar: userData.avatar,
                            });

                            sendToUser(currentUser.id, 'currentUser:update', {
                                avatar: userData.avatar,
                            });
                        } catch {
                            //
                        }
                    } else {
                        await new Promise<void>((resolve) => {
                            const chain = Chain([
                                createReadStream(databasePath),
                                parser(),
                                new StreamArray(),
                            ]);

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

                                            if (fileId) {
                                                value.file = { connect: { id: fileId } };
                                            }

                                            value.fileId = undefined;
                                            break;
                                        }
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
                                                        formatted: filesize(
                                                            created.size.toString(),
                                                        ),
                                                    };

                                                    const filePath = join(
                                                        uploadsPath,
                                                        created.fileName,
                                                    );

                                                    if (created.mimeType.startsWith('video/')) {
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
                                                                    created.thumbnailUrl =
                                                                        buildPublicUrl(
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

                                                    if (
                                                        ai.IMAGE_EMBEDDING_SUPPORTED_EXTENSIONS.includes(
                                                            extname(created.fileName),
                                                        )
                                                    ) {
                                                        const aiEnabled =
                                                            currentUser.aiSettings?.enabled ?? true;
                                                        if (aiEnabled) {
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
                                                                ]),
                                                            );
                                                        }
                                                    } else if (
                                                        ai.VIDEO_EMBEDDING_SUPPORTED_EXTENSIONS.includes(
                                                            extname(created.fileName),
                                                        )
                                                    ) {
                                                        const aiEnabled =
                                                            currentUser.aiSettings?.enabled ?? true;
                                                        if (aiEnabled) {
                                                            event.waitUntil(
                                                                Promise.all([
                                                                    enqueueAIJob({
                                                                        userId: currentUser.id,
                                                                        fileId: created.id,
                                                                        type: AIJobType.GenerateVideoEmbedding,
                                                                    }),
                                                                ]),
                                                            );
                                                        }
                                                    } else if (
                                                        ai.TEXT_EMBEDDING_SUPPORTED_EXTENSIONS.includes(
                                                            extname(created.fileName),
                                                        )
                                                    ) {
                                                        const aiEnabled =
                                                            currentUser.aiSettings?.enabled ?? true;
                                                        if (aiEnabled) {
                                                            event.waitUntil(
                                                                enqueueAIJob({
                                                                    userId: currentUser.id,
                                                                    fileId: created.id,
                                                                    type: AIJobType.GenerateTextEmbedding,
                                                                }),
                                                            );
                                                        }
                                                    }

                                                    await insert(fileSearchDb, {
                                                        id: created.id,
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
                                                    name: created.name,
                                                });
                                                break;
                                            case 'note':
                                                await insert(noteSearchDb, {
                                                    id: created.id,
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

                            chain.on('end', async () => {
                                await processing;
                                resolve();
                            });
                        });
                    }
                }

                await fsp.rm(tempPath, { recursive: true });
                await createLog(event, {
                    action: 'Load Backup',
                    message: `Loaded backup ${backupId}`,
                });
                await updateState(null);
            } catch {
                await createLog(event, {
                    action: 'Load Backup Failed',
                    message: `Failed to load backup ${backupId}`,
                });
                await updateState(null);
            }
        })(),
    );
});
