import { defu } from 'defu';

export default defineEventHandler(async (event) => {
    adminOnly(event);

    const users = await prisma.user.findMany({
        orderBy: {
            createdAt: 'desc',
        },
        select: {
            id: true,
            username: true,
            avatar: true,
            permissions: true,
            totpEnabled: true,
            superAdmin: true,
            limits: true,
            createdAt: true,
            domains: true,
            _count: {
                select: { files: true, folders: true, notes: true, codes: true },
            },
        },
    });

    return users.map((user) => ({
        id: user.id,
        username: user.username,
        avatar: user.avatar,
        permissions: user.permissions,
        totpEnabled: user.totpEnabled,
        superAdmin: user.superAdmin,
        limits: defu(user.limits, defaultUserLimits) as IUserLimits,
        createdAt: user.createdAt,
        domains: user.domains,
        stats: user._count,
    }));
});
