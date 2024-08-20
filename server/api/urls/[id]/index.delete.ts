import { sendToUser } from '~~/server/plugins/socketIO';

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;
    if (!currentUser) {
        throw createError({
            statusCode: 401,
            statusMessage: 'Unauthorized',
            message: 'You do not have permission to perform this action',
        });
    }

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

    if (findUrlById.authorId !== currentUser.id) {
        throw createError({
            statusCode: 403,
            statusMessage: 'Forbidden',
            message: 'You do not have permission to perform this action',
        });
    }

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
