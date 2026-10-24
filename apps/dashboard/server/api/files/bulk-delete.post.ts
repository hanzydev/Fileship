import z from 'zod';

const validationSchema = z.object({
    files: z.array(z.uuid()).min(1).max(5000),
    permanent: z.boolean().optional(),
});

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) throw invalidBodyError(body.error);

    const filesToBeDeleted = await prisma.file.findMany({
        where: {
            id: { in: body.data.files },
            authorId: currentUser.id,
            deletedAt: null,
        },
        select: {
            id: true,
            fileName: true,
            storageId: true,
            name: true,
            mimeType: true,
            size: true,
            password: true,
            piiDetected: true,
            expiresAt: true,
            aiAnalyzedAt: true,
            folder: { select: { public: true } },
        },
    });

    if (!filesToBeDeleted.length) {
        throw apiError(404, 'errorsFiles.noFilesToDelete');
    }

    const fileIds = filesToBeDeleted.map((file) => file.id);

    const isTrashed = !body.data.permanent && (await isTrashEnabled());

    if (isTrashed) {
        await prisma.file.updateMany({
            where: { id: { in: fileIds } },
            data: { deletedAt: new Date() },
        });
    } else {
        await deleteFilesForever(filesToBeDeleted);
    }

    emitFilesDeleted(event, currentUser.id, filesToBeDeleted, isTrashed ? 'trashed' : 'deleted');

    sendToUser(currentUser.id, 'file:bulkDelete', fileIds);
    sendToUser(currentUser.id, 'trash:update', null);

    await createLog(event, {
        action: 'Bulk Delete Files',
        message: isTrashed
            ? `Moved ${fileIds.length} files to trash`
            : `Deleted ${fileIds.length} files`,
    });
});
