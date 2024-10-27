export default defineEventHandler((event) => {
    userOnly(event);

    return prisma.credential.findMany({
        where: {
            userId: event.context.user!.id,
        },
        select: {
            id: true,
            name: true,
            createdAt: true,
        },
    });
});
