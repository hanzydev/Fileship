export default defineEventHandler(async (event) => {
    adminOnly(event);

    const findUserById = await prisma.user.findUnique({
        where: {
            id: getRouterParam(event, 'id'),
        },
        select: userListSelect,
    });

    if (!findUserById) {
        throw createError({
            statusCode: 404,
            message: 'User not found',
        });
    }

    return serializeListUser(findUserById);
});
