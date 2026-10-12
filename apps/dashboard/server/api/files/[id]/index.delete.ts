export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const fileId = getRouterParam(event, 'id');

    const findFileById = await prisma.file.findUnique({
        where: {
            id: fileId,
            authorId: currentUser.id,
        },
    });

    if (!findFileById) {
        throw createError({
            status: 404,
            message: 'File not found',
        });
    }

    await prisma.file.delete({
        where: {
            id: fileId,
        },
    });

    await removeFileData([findFileById]);

    await createLog(event, {
        action: 'Delete File',
        message: `Deleted file ${findFileById.fileName}`,
    });

    sendToUser(currentUser.id, 'file:delete', fileId);
});
