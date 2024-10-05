export default defineEventHandler((event) => {
    userOnly(event);

    const currentUser = event.context.user!;

    return prisma.note.findMany({
        where: {
            authorId: currentUser.id,
        },
        orderBy: {
            createdAt: 'desc',
        },
    });
});
