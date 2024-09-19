export default defineEventHandler((event) => {
    const currentUser = event.context.user;

    if (!currentUser) {
        throw createError({
            statusCode: 401,
            statusMessage: 'Unauthorized',
            message: 'You do not have permission to perform this action',
        });
    }

    return prisma.session.findMany({
        where: {
            userId: currentUser.id,
        },
        select: {
            id: true,
            ip: true,
            location: true,
            os: true,
            platform: true,
            lastSeen: true,
        },
        orderBy: {
            lastSeen: 'desc',
        },
    });
});
