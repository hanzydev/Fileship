export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;

    await prisma.session.delete({
        where: {
            id: currentUser.currentSessionId,
        },
    });

    await sendToSession(currentUser.id, currentUser.currentSessionId, 'logout', null);

    sendToUser(currentUser.id, 'session:delete', currentUser.currentSessionId);
});
