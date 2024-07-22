export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;
    if (!currentUser) {
        throw createError({
            statusCode: 401,
            statusMessage: 'Unauthorized',
            message: 'You do not have permission to perform this action',
        });
    }

    return prisma.user.findUnique({
        where: {
            id: currentUser.id,
        },
        select: {
            domains: true,
        },
    });
});
