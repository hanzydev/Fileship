export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const fileId = getRouterParam(event, 'id');

    const findFileById = await prisma.file.findUnique({
        where: {
            id: fileId,
            authorId: currentUser.id,
            deletedAt: null,
        },
    });

    if (!findFileById) {
        throw apiError(404, 'errorsFiles.fileNotFound');
    }

    const isTrashed = await isTrashEnabled();

    if (isTrashed) {
        await prisma.file.update({ where: { id: fileId }, data: { deletedAt: new Date() } });
    } else {
        await deleteFilesForever([findFileById]);
    }

    await createLog(event, {
        action: 'Delete File',
        message: isTrashed
            ? `Moved file ${findFileById.fileName} to trash`
            : `Deleted file ${findFileById.fileName}`,
    });

    sendToUser(currentUser.id, 'file:delete', fileId);
    sendToUser(currentUser.id, 'trash:update', null);
});
