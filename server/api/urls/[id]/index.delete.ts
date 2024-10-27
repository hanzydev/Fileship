export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const urlId = getRouterParam(event, 'id');

    const findUrlById = await prisma.url.findUnique({
        where: {
            id: urlId,
        },
    });

    if (!findUrlById) {
        throw createError({
            statusCode: 404,
            statusMessage: 'Not Found',
            message: 'URL not found',
        });
    }

    if (findUrlById.authorId !== currentUser.id) throw forbiddenError;

    await prisma.view.deleteMany({
        where: {
            urlId,
        },
    });

    await prisma.url.delete({
        where: {
            id: urlId,
        },
    });

    await createLog(event, {
        action: 'Delete URL',
        message: `Deleted URL ${findUrlById.vanity}`,
    });

    sendToUser(currentUser.id, 'delete:url', urlId);
});
