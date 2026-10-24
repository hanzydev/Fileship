import { randomUUID } from 'node:crypto';

import { nanoid } from 'nanoid';
import { basename, extname } from 'pathe';
import { z } from 'zod';

const validationSchema = z.object({
    uploadId: z
        .string()
        .regex(/^[\w-]{1,64}$/, 'errorsFiles.invalidUploadId')
        .optional(),
    totalChunks: z
        .number()
        .int()
        .min(1, 'errorsFiles.totalChunksMin')
        .max(10000, 'errorsFiles.tooManyChunks')
        .optional(),
    currentChunk: z.number().int().min(1, 'errorsFiles.currentChunkMin').optional(),
    chunkOffset: z.number().int().min(0, 'errorsFiles.chunkOffsetMin').optional(),
    fileNameType: z
        .union([z.literal('Random'), z.literal('UUID'), z.literal('Original')])
        .nullish(),
    compression: z
        .number()
        .min(0, 'errorsFiles.compressionMin')
        .max(100, 'errorsFiles.compressionMax')
        .optional(),
    password: z.string().max(48, 'errorsFiles.passwordMax').nullish(),
    maxViews: z.number().int().min(0, 'errorsFiles.maxViewsMin').nullish(),
    expiration: z.number().int().min(0, 'errorsFiles.expirationMin').nullish(),
    folderId: z.string().nullish(),
});

export default defineEventHandler(async (event) => {
    fileUploaderOnly(event);

    const currentUser = event.context.user!;
    const formData = await readFormData(event);

    const file = formData.get('file');
    if (!file) {
        throw apiError(400, 'errorsFiles.missingFile');
    }

    if (typeof file === 'string') {
        throw apiError(400, 'errorsFiles.invalidFile');
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

    if (!body.success) throw invalidBodyError(body.error);

    const isCompressible = isProcessableImage(file.type) && !!body.data.compression;
    const extensionName = isCompressible ? '.jpeg' : safeExtension(file.name);

    const assertFolder = async () => {
        if (!body.data.folderId) return;

        const findFolderById = await prisma.folder.findUnique({
            where: {
                id: body.data.folderId,
                authorId: currentUser.id,
                deletedAt: null,
            },
        });

        if (!findFolderById) {
            throw apiError(404, 'errorsFiles.folderNotFound');
        }
    };

    const _upload = await receiveUpload(
        currentUser,
        {
            kind: 'file',
            uploadId: body.data.uploadId ?? nanoid(8),
            part: body.data.currentChunk!,
            offset: body.data.chunkOffset ?? 0,
            total: body.data.totalChunks!,
            data: new Uint8Array(await file.arrayBuffer()),
            mimeType: file.type,
            compression: body.data.compression,
        },
        async () => {
            let fileName = `${fileNameBase(file.name) || nanoid(8)}${extensionName}`;

            if (body.data.fileNameType === 'Random') {
                fileName = `${nanoid(8)}${extensionName}`;
            } else if (body.data.fileNameType === 'UUID') {
                fileName = `${randomUUID()}${extensionName}`;
            } else {
                assertFileNameAllowed(fileName);
            }

            const findFileByFileName = await prisma.file.findUnique({
                where: {
                    fileName,
                },
            });

            if (findFileByFileName) {
                throw apiError(
                    409,
                    findFileByFileName.deletedAt
                        ? 'errorsFiles.nameInTrash'
                        : 'errorsFiles.nameExists',
                );
            }

            await assertFolder();

            return fileName;
        },
        async (data) => {
            await assertFolder();

            return prisma.file.create({
                data: {
                    ...data,
                    name: toDisplayName(
                        `${basename(file.name, extname(file.name))}${isCompressible ? '.jpeg' : extname(file.name)}`,
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
            });
        },
    );

    if (_upload) {
        const upload = {
            ..._upload,
            size: {
                raw: _upload.size.toString(),
                formatted: formatSize(_upload.size),
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

        await indexCreatedFile(event, currentUser, upload);

        emitFileUploaded(event, currentUser, _upload, 'upload');

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
