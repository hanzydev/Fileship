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

    await prisma.$transaction(async (tx) => {
        await tx.file.deleteMany({
            where: {
                id: { in: fileIds },
            },
        });
    });

    await removeFileData(filesToBeDeleted);

    sendToUser(currentUser.id, 'file:bulkDelete', fileIds);

    await createLog(event, {
        action: 'Bulk Delete Files',
        message: `Deleted ${fileIds.length} files`,
    });
});
