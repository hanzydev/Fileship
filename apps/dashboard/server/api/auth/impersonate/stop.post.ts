export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const adminToken = getCookie(event, 'adminToken');
    const adminSession = adminToken && (await findSession(adminToken));

    if (!adminSession || adminSession.id !== currentUser.impersonator?.sessionId) {
        throw createError({
            status: 400,
            message: 'You are not signed in as another user',
        });
    }

    await prisma.session.delete({
        where: {
            id: currentUser.currentSessionId,
        },
    });

    await logoutSockets((user) => user.currentSessionId === currentUser.currentSessionId);

    sendToUser(currentUser.id, 'session:delete', currentUser.currentSessionId);

    setTokenCookie(event, 'token', adminToken!);
    deleteCookie(event, 'adminToken');
});
