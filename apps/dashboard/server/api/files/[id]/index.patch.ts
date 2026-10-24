import { z } from 'zod';

import { AIJobType } from '#shared/prisma/enums';

const validationSchema = z
    .object({
        fileName: z
            .string()
            .transform((value) => toFileNameSlug(value))
            .pipe(
                z
                    .string()
                    .min(3, 'errorsFiles.fileNameMin')
                    .max(255, 'errorsFiles.fileNameMax')
                    .regex(/^[^.]/, 'errorsFiles.fileNameDot')
                    .refine((value) => !isUuid(value), 'errorsFiles.fileNameReserved'),
            )
            .optional(),
        name: z
            .string()
            .transform((value) => value.replace(/[\\/:*?"<>|\p{Cc}]/gu, '').trim())
            .pipe(z.string().min(1, 'errorsFiles.nameRequired').max(255, 'errorsFiles.nameMax'))
            .optional(),
        password: z.string().max(48, 'errorsFiles.passwordMax').nullish(),
        maxViews: z.number().int().min(0, 'errorsFiles.maxViewsMin').optional(),
        expiration: z.number().int().min(0, 'errorsFiles.expirationMin').nullish(),
        folderId: z
            .string()
            .nullish()
            .transform((value) => (value === '' ? null : value)),
        sensitive: z.boolean().nullable().optional(),
    })
    .strict();

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) throw invalidBodyError(body.error);

    const fileId = getRouterParam(event, 'id');

    const findFileById = await prisma.file.findUnique({
        where: {
            id: fileId,
            deletedAt: null,
        },
        include: { author: { select: { domains: true } } },
    });

    const access = findFileById && (await fileAccessOf(findFileById, currentUser.id));

    if (!findFileById || !access) {
        throw apiError(404, 'errorsFiles.fileNotFound');
    }

    if (
        access !== 'owner' &&
        (access === 'viewer' || Object.keys(body.data).some((key) => key !== 'name'))
    ) {
        throw forbiddenError();
    }

    if ('expiration' in body.data) {
        if (body.data.expiration) {
            (body.data as any).expiresAt = new Date(Date.now() + body.data.expiration);
        } else (body.data as any).expiresAt = null;

        delete body.data.expiration;
    }

    if ('sensitive' in body.data) {
        const { sensitive } = body.data;

        Object.assign(body.data, {
            piiOverride: sensitive,
            piiDetected: sensitive ?? findFileById.piiReasons.length > 0,
        });

        delete body.data.sensitive;
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
            throw apiError(404, 'errorsFiles.folderNotFound');
        }
    }

    if (body.data.fileName && body.data.fileName !== findFileById.fileName) {
        const findFileByFileName = await prisma.file.findUnique({
            where: {
                fileName: body.data.fileName,
            },
        });

        if (findFileByFileName) {
            throw apiError(
                409,
                findFileByFileName.deletedAt ? 'errorsFiles.nameInTrash' : 'errorsFiles.nameExists',
            );
        }

        await (
            await getStorage(findFileById.storageId)
        ).move(uploadKey(findFileById.fileName), uploadKey(body.data.fileName));
    }

    const _updatedFile = await prisma.file.update({
        where: {
            id: fileId,
        },
        include: {
            folder: { select: { id: true, name: true } },
            _count: { select: { views: true } },
        },
        data: body.data,
        omit: {
            embedding: true,
            textEmbedding: true,
            ocrText: true,
        },
    });

    const todayViews = await getTodayViews([_updatedFile.id]);
    const updatedFile = serializeFile(
        event,
        _updatedFile,
        findFileById.author.domains,
        todayViews.get(_updatedFile.id),
    );

    await upsertFileSearchEntry(updatedFile.id);

    if (
        (body.data as any).piiOverride === null &&
        (currentUser.aiSettings?.enabled ?? true) &&
        ['image', 'text'].includes(getAIFileKind(updatedFile.fileName)!)
    )
        event.waitUntil(
            enqueueAIJob({
                userId: currentUser.id,
                fileId: updatedFile.id,
                type: AIJobType.DetectPII,
            }),
        );

    await createLog(event, {
        action: 'Update File',
        message: `Updated file ${updatedFile.fileName}`,
    });

    await sendToFolder(
        findFileById.authorId,
        [findFileById.folderId, updatedFile.folderId],
        'file:update',
        updatedFile.folderId === findFileById.folderId
            ? { ...updatedFile, folderId: undefined, folder: undefined }
            : updatedFile,
    );

    return updatedFile;
});
