export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;

    await prisma.session.delete({
        where: {
            id: currentUser.impersonator?.sessionId ?? currentUser.currentSessionId,
        },
    });

    await logoutSockets((user) => user.currentSessionId === currentUser.currentSessionId);

    deleteCookie(event, 'token');
    deleteCookie(event, 'adminToken');

    sendToUser(currentUser.id, 'session:delete', currentUser.currentSessionId);
});
