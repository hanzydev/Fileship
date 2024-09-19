import { createReadStream, existsSync, promises as fsp } from 'node:fs';

import { nanoid } from 'nanoid';
import { extname, join } from 'pathe';
import Chain from 'stream-chain';
import parser from 'stream-json';
import StreamArray from 'stream-json/streamers/StreamArray.js';
import { extract } from 'tar';
import { z } from 'zod';

import { sendToUser } from '~~/server/plugins/socketIO';

const validationSchema = z
    .object({
        verificationData: z
            .string({
                invalid_type_error: 'Invalid verification data',
            })
            .optional(),
    })
    .optional();

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;

    if (!currentUser) {
        throw createError({
            statusCode: 401,
            statusMessage: 'Unauthorized',
            message: 'You do not have permission to perform this action',
        });
    }

    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw createError({
            statusCode: 400,
            statusMessage: 'Bad Request',
            message: 'Invalid body',
            data: body.error.format(),
        });
    }

    const backupId = getRouterParam(event, 'id');
    const backupPath = join(
        dataDirectory,
        'backups',
        currentUser.id,
        `${backupId}.tgz`,
    );

    if (!existsSync(backupPath)) {
        throw createError({
            statusCode: 404,
            statusMessage: 'Not Found',
            message: 'Backup not found',
        });
    }

    await verifySession(currentUser, body.data?.verificationData);

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
        await fsp.rm(join(uploadsPath, fileName), { force: true });
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

    extract({
        file: backupPath,
        cwd: tempPath,
    }).then(async () => {
        const databases = ['view', 'folder', 'note', 'code', 'url', 'file'];

        const backupUploadsPath = join(tempPath, 'uploads');
        const backupUploads = await fsp.readdir(backupUploadsPath);

        const renamedUploads = new Map<string, string>();

        for (const backupUpload of backupUploads) {
            const uploadPath = join(uploadsPath, backupUpload);

            if (existsSync(uploadPath)) {
                renamedUploads.set(
                    backupUpload,
                    `${nanoid(8)}${extname(backupUpload)}`,
                );
            } else {
                renamedUploads.set(backupUpload, backupUpload);
            }

            const renamed = renamedUploads.get(backupUpload)!;

            try {
                await fsp.rename(
                    join(backupUploadsPath, backupUpload),
                    join(uploadsPath, renamed),
                );
            } catch {
                //
            }
        }

        for (const database of databases) {
            await new Promise((resolve) => {
                const databasePath = join(
                    tempPath,
                    'database',
                    `${database}.json`,
                );

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

                    await (prisma as any)[database].create({
                        data: value,
                    });

                    sendToUser(currentUser.id, `create:${database}`, value);
                });

                chain.on('end', resolve);
            });
        }

        await fsp.rm(tempPath, { recursive: true });

        await createLog(event, {
            action: 'Load Backup',
            message: `Loaded backup ${backupId}`,
        });
    });
});
