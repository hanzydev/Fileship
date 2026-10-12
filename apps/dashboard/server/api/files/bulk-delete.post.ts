import z from 'zod';

const validationSchema = z.object({
    files: z.array(z.uuid()).min(1).max(5000),
});

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw createError({
            status: 400,
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

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
        },
    });

    if (!filesToBeDeleted.length) {
        throw createError({
            status: 404,
            message: 'No files found to delete',
        });
    }

    const fileIds = filesToBeDeleted.map((file) => file.id);

    const isTrashed = await isTrashEnabled();

    if (isTrashed) {
        await prisma.file.updateMany({
            where: { id: { in: fileIds } },
            data: { deletedAt: new Date() },
        });
    } else {
        await deleteFilesForever(filesToBeDeleted);
    }

    sendToUser(currentUser.id, 'file:bulkDelete', fileIds);
    sendToUser(currentUser.id, 'trash:update', null);

    await createLog(event, {
        action: 'Bulk Delete Files',
        message: isTrashed
            ? `Moved ${fileIds.length} files to trash`
            : `Deleted ${fileIds.length} files`,
    });
});
