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

    if (findCodeById.authorId !== currentUser.id) {
        throw createError({
            statusCode: 403,
            statusMessage: 'Forbidden',
            message: 'You do not have permission to perform this action',
        });
    }

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

    const log = await prisma.log.create({
        data: {
            action: 'Delete Code',
            userId: currentUser.id,
            message: `Deleted ${findCodeById.title}`,
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

    sendToUser(currentUser.id, 'delete:code', codeId);
    await sendByFilter(
        (socket) => isAdmin(socket.handshake.auth.user)!,
        'create:log',
        log,
    );
});
