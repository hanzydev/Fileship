export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, trashSelectionSchema.safeParse);

    if (!body.success) {
        throw createError({
            status: 400,
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    const trashed = { authorId: currentUser.id, deletedAt: { not: null } };

    const { files, folders, notes } = await deleteTrashed({
        files: { ...trashed, id: { in: body.data.files } },
        folders: { ...trashed, id: { in: body.data.folders } },
        notes: { ...trashed, id: { in: body.data.notes } },
    });

    await createLog(event, {
        action: 'Delete From Trash',
        message: `Deleted ${files.length} files, ${folders.length} folders and ${notes.length} notes forever`,
    });

    sendToUser(currentUser.id, 'trash:update', null);
});
