import defu from 'defu';

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

    const stats = await prisma.user.findUnique({
        where: {
            id: currentUser.id,
        },
        select: {
            _count: {
                select: { files: true, folders: true, notes: true, codes: true, urls: true },
            },
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
        limits: defu(currentUser.limits, defaultUserLimits) as IUserLimits,
        backupRestoreState: currentUser.backupRestoreState,
        theme: currentUser.theme,
        stats: stats!._count,
    };
});
