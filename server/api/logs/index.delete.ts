import { sendByFilter, sendToUser } from '~~/server/plugins/socketIO';
import { isAdmin } from '~~/utils/user';

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;
    if (!currentUser?.superAdmin) {
        throw createError({
            statusCode: 401,
            statusMessage: 'Unauthorized',
            message: 'You do not have permission to perform this action',
        });
    }

    await prisma.log.deleteMany({});

    await sendByFilter(
        (socket) => isAdmin(socket.handshake.auth.user)!,
        'delete:log:all',
        null,
    );

    await createLog(event, {
        action: 'Flush Logs',
        message: 'Flushed all logs',
    });
});
