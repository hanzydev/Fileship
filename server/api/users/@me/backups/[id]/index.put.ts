import { createReadStream, existsSync, promises as fsp } from 'node:fs';

import { nanoid } from 'nanoid';
import { extname, join } from 'pathe';
import Chain from 'stream-chain';
import parser from 'stream-json';
import StreamArray from 'stream-json/streamers/StreamArray.js';
import { extract } from 'tar';
import { z } from 'zod';

import { BackupRestoreState } from '@prisma/client';

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
            where: {
                id: currentUser.id,
            },
            data: {
                backupRestoreState: state,
            },
        });

        sendToUser(currentUser.id, 'update:currentUser', {
            backupRestoreState: state,
        });
    };

    await updateState(BackupRestoreState.DeletingPreviousData);

    const userFiles = await prisma.file.findMany({
        where: {
            authorId: currentUser.id,
        },
        select: {
            fileName: true,
        },
    });

    const uploadsPath = join(dataDirectory, 'uploads');

    for (const { fileName } of userFiles) {
        await fsp.rm(join(uploadsPath, fileName), { force: true }).catch(() => null);
    }

    await prisma.$transaction([
        prisma.view.deleteMany({
            where: {
                OR: [
                    {
                        file: {
                            authorId: currentUser.id,
                        },
                    },
                    {
                        code: {
                            authorId: currentUser.id,
                        },
                    },
                    {
                        url: {
                            authorId: currentUser.id,
                        },
                    },
                ],
            },
        }),
        prisma.folder.deleteMany({
            where: {
                authorId: currentUser.id,
            },
        }),
        prisma.note.deleteMany({
            where: {
                authorId: currentUser.id,
            },
        }),
        prisma.code.deleteMany({
            where: {
                authorId: currentUser.id,
            },
        }),
        prisma.url.deleteMany({
            where: {
                authorId: currentUser.id,
            },
        }),
        prisma.file.deleteMany({
            where: {
                authorId: currentUser.id,
            },
        }),
    ]);

    sendToUser(currentUser.id, 'delete:all', null);

    const tempPath = join(dataDirectory, 'temp', backupId!);
    await fsp.mkdir(tempPath);

    await updateState(BackupRestoreState.Extracting);

    extract({
        file: backupPath,
        cwd: tempPath,
    }).then(async () => {
        const databases = ['folder', 'note', 'code', 'url', 'file', 'user', 'view'];

        const backupUploadsPath = join(tempPath, 'uploads');
        const backupUploads = await fsp.readdir(backupUploadsPath);

        const renamedUploads = new Map<string, string>();

        for (const backupUpload of backupUploads) {
            const uploadPath = join(uploadsPath, backupUpload);

            if (existsSync(uploadPath)) {
                renamedUploads.set(backupUpload, `${nanoid(8)}${extname(backupUpload)}`);
            } else {
                renamedUploads.set(backupUpload, backupUpload);
            }

            const renamed = renamedUploads.get(backupUpload)!;

            try {
                await fsp.rename(join(backupUploadsPath, backupUpload), join(uploadsPath, renamed));
            } catch {
                //
            }
        }

        await updateState(BackupRestoreState.RestoringData);

        for (const database of databases) {
            const databasePath = join(tempPath, 'database', `${database}.json`);

            if (database === 'user') {
                try {
                    const userData = JSON.parse(
                        await fsp.readFile(databasePath, { encoding: 'utf-8' }),
                    );

                    await prisma.user.update({
                        where: {
                            id: currentUser.id,
                        },
                        data: userData,
                    });

                    sendToUser(currentUser.id, 'update:domains', userData.domains);
                    sendToUser(currentUser.id, 'update:embed', userData.embed);

                    await sendByFilter((user) => isAdmin(user), 'update:user', {
                        id: currentUser.id,
                        avatar: userData.avatar,
                    });

                    sendToUser(currentUser.id, 'update:currentUser', {
                        avatar: userData.avatar,
                    });
                } catch {
                    //
                }
            } else {
                await new Promise((resolve) => {
                    const chain = Chain([
                        createReadStream(databasePath),
                        parser(),
                        new StreamArray(),
                    ] as const);

                    chain.on('data', async ({ value }) => {
                        if (value.authorId) value.authorId = currentUser.id;
                        if (value.fileName) {
                            value.fileName = renamedUploads.get(value.fileName);
                        }

                        try {
                            await (prisma as any)[database].create({
                                data: value,
                            });

                            sendToUser(currentUser.id, `create:${database}`, value);
                        } catch {
                            //
                        }
                    });

                    chain.on('end', resolve);
                });
            }
        }

        await fsp.rm(tempPath, { recursive: true });

        await createLog(event, {
            action: 'Load Backup',
            message: `Loaded backup ${backupId}`,
        });

        await updateState(null);
    });
});
