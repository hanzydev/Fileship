import { sendByFilter, sendToUser } from '~~/server/plugins/socketIO';
import { isAdmin } from '~~/utils/user';

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

    const log = await prisma.log.create({
        data: {
            action: 'Delete URL',
            userId: currentUser.id,
            message: `Deleted ${findUrlById.vanity}`,
            ip: getRequestIP(event, { xForwardedFor: true })!,
        },
        include: {
            user: {
                select: {
                    id: true,
                    username: true,
                },
            },
        },
    });

    sendToUser(currentUser.id, 'delete:url', urlId);
    await sendByFilter(
        (socket) => isAdmin(socket.handshake.auth.user)!,
        'create:log',
        log,
    );
});
