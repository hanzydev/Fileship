import type { H3Event } from 'h3';
import { nanoid } from 'nanoid';
import { z } from 'zod';

import type { Inbox, User } from '~~/generated/prisma/client';

const validationSchema = z.object({
    uploadId: z
        .string()
        .regex(/^[\w-]{1,64}$/, 'errorsContent.invalidUploadId')
        .optional(),
    totalChunks: z
        .number()
        .int()
        .min(1, 'errorsContent.totalChunksMin')
        .max(10000, 'errorsContent.tooManyChunks')
        .optional(),
    currentChunk: z.number().int().min(1, 'errorsContent.currentChunkMin').optional(),
    chunkOffset: z.number().int().min(0, 'errorsContent.chunkOffsetMin').optional(),
    inboxPassword: z.string().max(48, 'errorsContent.inboxPasswordMax').nullish(),
});

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;

    await rateLimit(`inboxUpload:${getClientIp(event)}`, 120, 60 * 1000);

    const findInboxById = await prisma.inbox.findFirst({
        where: {
            id: getRouterParam(event, 'id'),
        },
        include: {
            user: true,
        },
    });

    if (!findInboxById?.enabled || !canUploadFiles(findInboxById.user)) {
        throw apiError(404, 'errorsContent.inboxNotFound');
    }

    const limit = usableSpaceOf(findInboxById.user);
    const formData = await readUploadForm(event, {
        field: 'file',
        memoryLimit: getSettingsSync().fileChunkSize * 1024 * 1024,
        spill: true,
        maxSize:
            limit === null ? null : Number(limit - (await storageUsedBy(findInboxById.user.id))),
        maxSizeError: () =>
            apiError(400, 'errorsFiles.storageLimitReached', undefined, { quotaExceeded: true }),
    });

    try {
        return await saveInboxUpload(event, currentUser, findInboxById, formData);
    } finally {
        await removeUploadForm(formData);
    }
});

const saveInboxUpload = async (
    event: H3Event,
    currentUser: H3Event['context']['user'],
    findInboxById: Inbox & { user: User },
    formData: UploadForm,
) => {
    const file = formData.file;
    if (!file) {
        throw apiError(400, 'errorsContent.missingFile');
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
        throw invalidBodyError(body.error);
    }

    if (
        currentUser?.id !== findInboxById.user.id &&
        findInboxById.password &&
        !(await limitPasswordAttempts(
            event,
            findInboxById.id,
            findInboxById.password === body.data.inboxPassword,
        ))
    ) {
        throw forbiddenError();
    }

    const extensionName = safeExtension(file.name);
    const mimeType = uploadMimeType(file);

    const _upload = await receiveUpload(
        findInboxById.user,
        {
            kind: `inbox:${findInboxById.id}`,
            uploadId: body.data.uploadId ?? nanoid(8),
            part: body.data.currentChunk!,
            offset: body.data.chunkOffset ?? 0,
            total: body.data.totalChunks!,
            data: file.data,
            mimeType,
        },
        async () => {
            const base = fileNameBase(file.name);
            const fileName = `${base ? `${base}-${nanoid(6)}` : nanoid(8)}${extensionName}`;

            const findFileByFileName = await prisma.file.findUnique({
                where: {
                    fileName,
                },
            });

            if (findFileByFileName) {
                throw apiError(409, 'errorsContent.fileNameTaken');
            }

            return fileName;
        },
        (data) =>
            prisma.file.create({
                data: {
                    ...data,
                    name: toDisplayName(file.name, data.fileName),
                    mimeType,
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
            }),
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
            directUrl: buildPublicUrl(event, findInboxById.user.domains, `/u/${_upload.fileName}`),
            embedUrl: buildPublicUrl(
                event,
                findInboxById.user.domains,
                `/view/${_upload.fileName}`,
            ),
            thumbnailUrl: getThumbnailUrl(event, findInboxById.user.domains, _upload),
        };

        await indexCreatedFile(event, findInboxById.user, upload);

        emitFileUploaded(event, findInboxById.user, _upload, 'inbox');

        await createLog(event, {
            action: 'Inbox File Upload',
            message: `File ${_upload.fileName} uploaded to inbox`,
        });

        await sendToFolder(findInboxById.user.id, [findInboxById.folderId], 'file:create', upload);
    }
};
