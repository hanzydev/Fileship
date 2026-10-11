export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;

    await prisma.session.delete({
        where: {
            id: currentUser.currentSessionId,
        },
    });

    await logoutSockets((user) => user.currentSessionId === currentUser.currentSessionId);

    sendToUser(currentUser.id, 'session:delete', currentUser.currentSessionId);
});
