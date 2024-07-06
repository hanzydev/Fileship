export default defineEventHandler((event) => {
    const currentUser = event.context.user;

    if (!currentUser) {
        throw createError({
            statusCode: 401,
            statusMessage: 'Unauthorized',
            message: 'You do not have permission to perform this action',
        });
    }

    return prisma.note.findMany({
        where: {
            authorId: currentUser.id,
        },
        orderBy: {
            createdAt: 'desc',
        },
    });
});
