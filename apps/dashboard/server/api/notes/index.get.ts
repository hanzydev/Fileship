export default defineEventHandler((event) => {
    userOnly(event);

    return prisma.note.findMany({
        where: {
            authorId: event.context.user!.id,
        },
        orderBy: {
            createdAt: 'desc',
        },
    });
});
