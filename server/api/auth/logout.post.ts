import { sendToSession, sendToUser } from '~~/server/plugins/socketIO';

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;

    if (!currentUser) {
        throw createError({
            statusCode: 401,
            statusMessage: 'Unauthorized',
            message: 'You do not have permission to perform this action',
        });
    }

    await prisma.session.delete({
        where: {
            id: currentUser.currentSessionId,
        },
    });

    await sendToSession(
        currentUser.id,
        currentUser.currentSessionId,
        'logout',
        null,
    );
    sendToUser(currentUser.id, 'delete:session', currentUser.currentSessionId);
});
