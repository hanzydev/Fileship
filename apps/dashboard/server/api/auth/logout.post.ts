export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const sessionIds = [currentUser.currentSessionId, currentUser.impersonator?.sessionId];

    const deleted = await prisma.session.delete({
        where: {
            id: currentUser.impersonator?.sessionId ?? currentUser.currentSessionId,
        },
        select: { id: true, userId: true },
    });

    await logoutSockets((user) => sessionIds.includes(user.currentSessionId));

    deleteCookie(event, 'token');
    deleteCookie(event, 'adminToken');

    sendToUser(currentUser.id, 'session:delete', currentUser.currentSessionId);
    if (deleted.id !== currentUser.currentSessionId)
        sendToUser(deleted.userId, 'session:delete', deleted.id);
});
