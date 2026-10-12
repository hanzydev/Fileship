import { promises as fsp } from 'node:fs';

import { filesize } from 'filesize';
import { nanoid } from 'nanoid';
import { basename, extname, join } from 'pathe';
import sharp from 'sharp';
import { z } from 'zod';

import { insert } from '@orama/orama';

import { AIJobType } from '#shared/prisma/enums';

const validationSchema = z.object({
    uploadId: z
        .string()
        .regex(/^[\w-]{1,64}$/, 'Invalid upload id')
        .optional(),
    totalChunks: z.number().min(1, 'Total chunks must be at least 1').optional(),
    currentChunk: z.number().min(1, 'Current chunk must be at least 1').optional(),
    chunkOffset: z.number().int().min(0, 'Chunk offset must be at least 0').optional(),
    inboxPassword: z.string().max(48, 'Inbox password must be at most 48 characters').nullish(),
});

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;
    const formData = await readFormData(event);

    const file = formData.get('file');
    if (!file) {
        throw createError({
            status: 400,
            message: 'Missing file',
        });
    }

    if (typeof file === 'string') {
        throw createError({
            status: 400,
            message: 'Invalid file',
        });
    }

    const chunkOffset = formData.get('chunkOffset');
    const uploadId = formData.get('uploadId');

    const body = validationSchema.safeParse({
        uploadId: typeof uploadId === 'string' ? uploadId : undefined,
        currentChunk: +formData.get('currentChunk')! || 1,
        totalChunks: +formData.get('totalChunks')! || 1,
        chunkOffset: chunkOffset === null ? undefined : +chunkOffset,
        inboxPassword: formData.get('inboxPassword') as string,
    });

    if (!body.success) {
        throw createError({
            status: 400,
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
            status: 404,
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

    const isProcessableImage =
        file.type.startsWith('image/') && !['image/gif', 'image/svg+xml'].includes(file.type);

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
            status: 409,
            message: 'A file with that name already exists',
        });
    }

    const sanitizedOriginalName = file.name.replace(/[^a-zA-Z0-9-_.]/g, '');
    const uploadSessionId = body.data.uploadId ?? nanoid(8);

    const tempPath = join(
        dataDirectory,
        'temp',
        findInboxById.user.id,
        `${uploadSessionId}_${sanitizedOriginalName}`,
    );

    await fsp.mkdir(join(dataDirectory, 'temp', findInboxById.user.id), { recursive: true });

    const buffer = new Uint8Array(await file.arrayBuffer());
    const { removeExifData } = await getSettings();
    const isLastChunk = body.data.currentChunk === body.data.totalChunks;

    const usableSpace = (findInboxById.user.limits as any).usableSpace as number;

    const totalSize = await prisma.file.aggregate({
        where: {
            authorId: findInboxById.user.id,
        },
        _sum: {
            size: true,
        },
    });

    const exceedsLimit = (size: number) =>
        !isAdmin(findInboxById.user) &&
        usableSpace > -1 &&
        (totalSize._sum.size ?? 0n) + BigInt(size) > BigInt(usableSpace * 1024 * 1024);

    if (exceedsLimit((body.data.chunkOffset ?? 0) + buffer.length)) {
        await fsp.rm(tempPath, { force: true });
        throw createError({
            status: 400,
            message: 'Storage limit reached',
        });
    }

    if (body.data.chunkOffset !== undefined) {
        await writeUploadChunk(tempPath, buffer, body.data.chunkOffset, isLastChunk);
    } else {
        await fsp.writeFile(tempPath, buffer);
    }

    if (isLastChunk) {
        if (isProcessableImage) {
            try {
                const temp2Path = join(
                    dataDirectory,
                    'temp',
                    findInboxById.user.id,
                    `${nanoid(8)}${extname(file.name)}`,
                );

                let imagePipeline = sharp(tempPath).rotate();
                if (!removeExifData) imagePipeline = imagePipeline.keepExif();

                await imagePipeline.toFile(temp2Path);
                await fsp.rename(temp2Path, tempPath);
            } catch {
                //
            }
        }

        const fileSize = (await fsp.stat(tempPath)).size;

        if (exceedsLimit(fileSize)) {
            await fsp.rm(tempPath).catch(() => {});
            throw createError({
                status: 400,
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

        await processMedia(_upload);

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
            thumbnailUrl: getThumbnailUrl(event, findInboxById.user.domains, _upload),
        };

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
            authorId: upload.authorId,
            fileName: upload.fileName,
            mimeType: upload.mimeType,
        });

        await createLog(event, {
            action: 'Inbox File Upload',
            message: `File ${fileName} uploaded to inbox`,
        });

        sendToUser(findInboxById.user.id, 'file:create', upload);
    }
});
