export default defineEventHandler((event) => {
    userOnly(event);

    const currentUser = event.context.user!;

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
