export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const codeId = getRouterParam(event, 'id');

    const findCodeById = await prisma.code.findUnique({
        where: {
            id: codeId,
        },
    });

    if (!findCodeById) {
        throw createError({
            statusCode: 404,
            statusMessage: 'Not Found',
            message: 'Code not found',
        });
    }

    if (findCodeById.authorId !== currentUser.id) throw forbiddenError;

    await prisma.view.deleteMany({
        where: {
            codeId,
        },
    });

    await prisma.code.delete({
        where: {
            id: codeId,
        },
    });

    await createLog(event, {
        action: 'Delete Code',
        message: `Deleted code ${findCodeById.title}`,
    });

    sendToUser(currentUser.id, 'delete:code', codeId);
});
