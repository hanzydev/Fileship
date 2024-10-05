export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;

    await prisma.session.update({
        where: {
            id: currentUser.currentSessionId,
        },
        data: {
            lastSeen: new Date(),
        },
    });

    return {
        id: currentUser.id,
        username: currentUser.username,
        avatar: currentUser.avatar,
        permissions: currentUser.permissions,
        createdAt: currentUser.createdAt,
        totpEnabled: currentUser.totpEnabled,
        currentSessionId: currentUser.currentSessionId,
        superAdmin: currentUser.superAdmin,
        limits: currentUser.limits,
    };
});
