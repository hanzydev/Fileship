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

    const extensionName = extname(file.name);

    const _upload = await receiveUpload(
        findInboxById.user,
        {
            uploadId: body.data.uploadId ?? nanoid(8),
            part: body.data.currentChunk!,
            offset: body.data.chunkOffset ?? 0,
            isLast: body.data.currentChunk === body.data.totalChunks,
            data: new Uint8Array(await file.arrayBuffer()),
            mimeType: file.type,
        },
        async () => {
            const fileName = `${basename(file.name.replace(/[^a-zA-Z0-9-_.]/g, ''), extensionName)}-${nanoid(6)}${
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

            return fileName;
        },
        (data) =>
            prisma.file.create({
                data: {
                    ...data,
                    mimeType: file.type,
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
        if (aiEnabled) event.waitUntil(enqueueFileAnalysis(upload));

        await upsertFileSearchEntry(upload.id);

        await createLog(event, {
            action: 'Inbox File Upload',
            message: `File ${_upload.fileName} uploaded to inbox`,
        });

        sendToUser(findInboxById.user.id, 'file:create', upload);
    }
});
