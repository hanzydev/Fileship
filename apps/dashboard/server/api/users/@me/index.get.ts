import { defu } from 'defu';

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;

    const stats = await prisma.user.findUnique({
        where: {
            id: currentUser.id,
        },
        select: {
            _count: {
                select: userCountSelect,
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
        impersonator: currentUser.impersonator?.username ?? null,
        superAdmin: currentUser.superAdmin,
        backupRestoreState: currentUser.backupRestoreState,
        theme: currentUser.theme,
        aiSettings: defu(currentUser.aiSettings, defaultUserAiSettings) as IUserAiSettings,
        stats: stats!._count,
    };
});
