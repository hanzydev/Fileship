import { randomUUID } from 'node:crypto';

import { filesize } from 'filesize';
import { nanoid } from 'nanoid';
import { basename, extname } from 'pathe';
import { z } from 'zod';

const validationSchema = z.object({
    uploadId: z
        .string()
        .regex(/^[\w-]{1,64}$/, 'Invalid upload id')
        .optional(),
    totalChunks: z.number().min(1, 'Total chunks must be at least 1').optional(),
    currentChunk: z.number().min(1, 'Current chunk must be at least 1').optional(),
    chunkOffset: z.number().int().min(0, 'Chunk offset must be at least 0').optional(),
    fileNameType: z
        .union([z.literal('Random'), z.literal('UUID'), z.literal('Original')])
        .nullish(),
    compression: z
        .number()
        .min(0, 'Compression must be at least 0')
        .max(100, 'Compression must be at most 100')
        .optional(),
    password: z.string().max(48, 'Password must be at most 48 characters').nullish(),
    maxViews: z.number().min(0, 'Max views must be at least 0').nullish(),
    expiration: z.number().min(0, 'Expiration must be at least 0').nullish(),
    folderId: z.string().nullish(),
});

export default defineEventHandler(async (event) => {
    fileUploaderOnly(event);

    const currentUser = event.context.user!;
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
        fileNameType: formData.get('fileNameType'),
        currentChunk: +formData.get('currentChunk')! || 1,
        totalChunks: +formData.get('totalChunks')! || 1,
        chunkOffset: chunkOffset === null ? undefined : +chunkOffset,
        maxViews: +formData.get('maxViews')!,
        password: formData.get('password') as string,
        expiration: +formData.get('expiration')!,
        compression: +formData.get('compression')!,
        folderId: formData.get('folderId') as string,
    });

    if (!body.success) {
        throw createError({
            status: 400,
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    const isCompressible = isProcessableImage(file.type) && !!body.data.compression;
    const extensionName = isCompressible ? '.jpeg' : extname(file.name);

    const _upload = await receiveUpload(
        currentUser,
        {
            uploadId: body.data.uploadId ?? nanoid(8),
            part: body.data.currentChunk!,
            offset: body.data.chunkOffset ?? 0,
            isLast: body.data.currentChunk === body.data.totalChunks,
            data: new Uint8Array(await file.arrayBuffer()),
            mimeType: file.type,
            compression: body.data.compression,
        },
        async () => {
            let fileName = `${basename(file.name.replace(/[^a-zA-Z0-9-_.]/g, ''), extname(file.name))}${
                extensionName
            }`;

            if (body.data.fileNameType === 'Random') {
                fileName = `${nanoid(8)}${extensionName}`;
            } else if (body.data.fileNameType === 'UUID') {
                fileName = `${randomUUID()}${extensionName}`;
            }

            if (!fileName.replaceAll('.', '') || fileName.startsWith('.')) {
                throw createError({
                    status: 400,
                    message: 'Invalid file name',
                });
            }

            const findFileByFileName = await prisma.file.findUnique({
                where: {
                    fileName,
                },
            });

            if (findFileByFileName) {
                throw createError({
                    status: 409,
                    message: findFileByFileName.deletedAt
                        ? 'A file with that name is in the trash'
                        : 'A file with that name already exists',
                });
            }

            if (body.data.folderId) {
                const findFolderById = await prisma.folder.findUnique({
                    where: {
                        id: body.data.folderId,
                        authorId: currentUser.id,
                        deletedAt: null,
                    },
                });

                if (!findFolderById) {
                    throw createError({
                        status: 404,
                        message: 'Folder not found',
                    });
                }
            }

            return fileName;
        },
        (data) =>
            prisma.file.create({
                data: {
                    ...data,
                    name: toDisplayName(
                        `${basename(file.name, extname(file.name))}${extensionName}`,
                        data.fileName,
                    ),
                    mimeType: isCompressible ? 'image/jpeg' : file.type,
                    password: body.data.password || null,
                    maxViews: body.data.maxViews || 0,
                    expiresAt: body.data.expiration
                        ? new Date(Date.now() + body.data.expiration)
                        : null,
                    authorId: currentUser.id,
                    folderId: body.data.folderId || null,
                },
                include: {
                    views: true,
                },
                omit: {
                    embedding: true,
                    textEmbedding: true,
                    ocrText: true,
                },
            }),
    );

    if (_upload) {
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
            directUrl: buildPublicUrl(event, currentUser.domains, `/u/${_upload.fileName}`),
            embedUrl: buildPublicUrl(event, currentUser.domains, `/view/${_upload.fileName}`),
            thumbnailUrl: getThumbnailUrl(event, currentUser.domains, _upload),
        };

        const aiEnabled = currentUser.aiSettings?.enabled ?? true;
        if (aiEnabled) event.waitUntil(enqueueFileAnalysis(upload));

        await upsertFileSearchEntry(upload.id);

        await createLog(event, {
            action: 'Upload File',
            message: `Uploaded file ${_upload.fileName}`,
        });

        sendToUser(currentUser.id, 'file:create', upload);

        const isText = TEXT_FILE_TYPES.some(
            (type) => type.extension === extname(upload.fileName).slice(1),
        );

        return {
            ...upload,
            url: buildPublicUrl(
                event,
                currentUser.domains,
                `/${currentUser.embed.enabled || isText ? 'view' : 'u'}/${_upload.fileName}`,
            ),
        };
    }
});
