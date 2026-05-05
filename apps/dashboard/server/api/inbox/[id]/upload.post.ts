import { existsSync, promises as fsp } from 'node:fs';

import { filesize } from 'filesize';
import fluentFfmpeg from 'fluent-ffmpeg';
import { nanoid } from 'nanoid';
import { basename, extname, join } from 'pathe';
import sharp from 'sharp';
import { z } from 'zod';

import ffmpeg from '@ffmpeg-installer/ffmpeg';
import { insert } from '@orama/orama';

import { AIJobType } from '#shared/prisma/enums';

fluentFfmpeg.setFfmpegPath(ffmpeg.path);

const validationSchema = z.object({
    totalChunks: z.number().min(1, 'Total chunks must be at least 1').optional(),
    currentChunk: z.number().min(1, 'Current chunk must be at least 1').optional(),
    inboxPassword: z.string().max(48, 'Inbox password must be at most 48 characters').nullish(),
});

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;
    const formData = await readFormData(event);

    const file = formData.get('file');
    if (!file) {
        throw createError({
            statusCode: 400,
            message: 'Missing file',
        });
    }

    if (typeof file === 'string') {
        throw createError({
            statusCode: 400,
            message: 'Invalid file',
        });
    }

    const body = validationSchema.safeParse({
        currentChunk: +formData.get('currentChunk')! || 1,
        totalChunks: +formData.get('totalChunks')! || 1,
        inboxPassword: formData.get('inboxPassword') as string,
    });

    if (!body.success) {
        throw createError({
            statusCode: 400,
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    const inboxId = getRouterParam(event, 'id');

    const findInboxById = await prisma.inbox.findFirst({
        where: {
            id: inboxId,
        },
        include: {
            user: true,
        },
    });

    if (!findInboxById?.enabled) {
        throw createError({
            statusCode: 404,
            message: 'Inbox not found',
        });
    }

    if (
        currentUser?.id !== findInboxById.user.id &&
        findInboxById.password &&
        findInboxById.password !== body.data.inboxPassword
    ) {
        throw forbiddenError;
    }

    const extensionName = extname(file.name);
    const fileName = `${basename(file.name.replace(/[^a-zA-Z0-9-_.]/g, ''), extname(file.name))}-${nanoid(6)}${
        extensionName
    }`;

    const findFileByFileName = await prisma.file.findUnique({
        where: {
            fileName,
        },
    });

    if (findFileByFileName) {
        throw createError({
            statusCode: 409,
            message: 'A file with that name already exists',
        });
    }

    const tempPath = join(
        dataDirectory,
        'temp',
        findInboxById.user.id,
        file.name.replace(/[^a-zA-Z0-9-_.]/g, ''),
    );

    if (!existsSync(join(dataDirectory, 'temp', findInboxById.user.id))) {
        await fsp.mkdir(join(dataDirectory, 'temp', findInboxById.user.id));
    }

    const buffer = new Uint8Array(await file.arrayBuffer());

    const removeExifData = (process.env.REMOVE_EXIF_DATA || 'true') === 'true';

    if (body.data.currentChunk === body.data.totalChunks) {
        if (existsSync(tempPath)) {
            await fsp.appendFile(tempPath, buffer);
        } else {
            await fsp.writeFile(tempPath, buffer);
        }

        if (file.type.startsWith('image/') && file.type !== 'image/gif') {
            try {
                const temp2Path = join(
                    dataDirectory,
                    'temp',
                    findInboxById.user.id,
                    `${nanoid(8)}${extname(file.name)}`,
                );

                const image = sharp(tempPath);

                if (removeExifData) {
                    await image.rotate().toFile(temp2Path);
                    await fsp.rename(temp2Path, tempPath);
                }
            } catch {
                //
            }
        }

        const fileSize = (await fsp.stat(tempPath)).size;
        const totalSize = await prisma.file.aggregate({
            where: {
                authorId: findInboxById.user.id,
            },
            _sum: {
                size: true,
            },
        });

        if (
            !isAdmin(findInboxById.user) &&
            (findInboxById.user.limits as any).usableSpace > -1 &&
            (totalSize._sum.size ?? 0n) + BigInt(fileSize) >
                BigInt((findInboxById.user.limits as any).usableSpace * 1024 * 1024)
        ) {
            throw createError({
                statusCode: 400,
                message: 'Storage limit reached',
            });
        }

        const filePath = join(dataDirectory, 'uploads', fileName);

        await moveFileRobust(tempPath, filePath);

        const _upload = await prisma.file.create({
            data: {
                fileName,
                mimeType: file.type,
                size: fileSize,
                authorId: findInboxById.user.id,
                folderId: findInboxById.folderId,
            },
            include: {
                views: true,
            },
            omit: {
                embedding: true,
                textEmbedding: true,
                ocrText: true,
            },
        });

        const upload = {
            ..._upload,
            size: {
                raw: _upload.size.toString(),
                formatted: filesize(_upload.size),
            },
            views: {
                total: _upload.views.length,
                today: _upload.views.filter((view) => {
                    const now = new Date();

                    return (
                        view.createdAt.getDate() === now.getDate() &&
                        view.createdAt.getMonth() === now.getMonth() &&
                        view.createdAt.getFullYear() === now.getFullYear()
                    );
                }).length,
            },
            directUrl: buildPublicUrl(event, findInboxById.user.domains, `/u/${_upload.fileName}`),
            embedUrl: buildPublicUrl(
                event,
                findInboxById.user.domains,
                `/view/${_upload.fileName}`,
            ),
            thumbnailUrl: undefined as string | null | undefined,
        };

        if (file.type.startsWith('video/')) {
            const thumbnailPath = join(dataDirectory, 'thumbnails', `${upload.id}.jpeg`);

            await new Promise<void>((resolve) => {
                fluentFfmpeg(filePath)
                    .videoFilters('thumbnail')
                    .frames(1)
                    .format('mjpeg')
                    .output(thumbnailPath)
                    .on('end', () => resolve())
                    .on('error', () => resolve())
                    .run();
            });

            upload.thumbnailUrl = existsSync(thumbnailPath)
                ? buildPublicUrl(
                      event,
                      findInboxById.user.domains,
                      `/u/${_upload.fileName}/thumbnail`,
                  )
                : null;
        }

        const aiEnabled = (findInboxById.user.aiSettings as any)?.enabled ?? true;
        if (aiEnabled) {
            if (ai.IMAGE_EMBEDDING_SUPPORTED_EXTENSIONS.includes(extensionName)) {
                event.waitUntil(
                    Promise.all([
                        enqueueAIJob({
                            userId: findInboxById.user.id,
                            fileId: upload.id,
                            type: AIJobType.GenerateClipEmbedding,
                        }),
                        enqueueAIJob({
                            userId: findInboxById.user.id,
                            fileId: upload.id,
                            type: AIJobType.GenerateOcrText,
                        }),
                        enqueueAIJob({
                            userId: findInboxById.user.id,
                            fileId: upload.id,
                            type: AIJobType.GenerateImageCaption,
                        }),
                        enqueueAIJob({
                            userId: findInboxById.user.id,
                            fileId: upload.id,
                            type: AIJobType.DetectPII,
                        }),
                    ]),
                );
            } else if (ai.VIDEO_EMBEDDING_SUPPORTED_EXTENSIONS.includes(extensionName)) {
                event.waitUntil(
                    Promise.all([
                        enqueueAIJob({
                            userId: findInboxById.user.id,
                            fileId: upload.id,
                            type: AIJobType.GenerateVideoEmbedding,
                        }),
                    ]),
                );
            } else if (ai.TEXT_EMBEDDING_SUPPORTED_EXTENSIONS.includes(extensionName)) {
                event.waitUntil(
                    Promise.all([
                        enqueueAIJob({
                            userId: findInboxById.user.id,
                            fileId: upload.id,
                            type: AIJobType.GenerateTextEmbedding,
                        }),
                        enqueueAIJob({
                            userId: findInboxById.user.id,
                            fileId: upload.id,
                            type: AIJobType.DetectPII,
                        }),
                    ]),
                );
            }
        }

        await insert(fileSearchDb, {
            id: upload.id,
            fileName: upload.fileName,
            mimeType: upload.mimeType,
        });

        await createLog(event, {
            action: 'Inbox File Upload',
            message: `File ${fileName} uploaded to inbox`,
        });

        sendToUser(findInboxById.user.id, 'file:create', upload);
    } else if (body.data.currentChunk === 1) {
        await fsp.writeFile(tempPath, buffer);
    } else {
        await fsp.appendFile(tempPath, buffer);
    }
});
