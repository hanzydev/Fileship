export default defineEventHandler(async (event) => {
    adminOnly(event);

    const findUserById = await prisma.user.findUnique({
        where: {
            id: getRouterParam(event, 'id'),
        },
        select: userListSelect,
    });

    if (!findUserById) {
        throw apiError(404, 'errorsUsers.userNotFound');
    }

    return serializeListUser(findUserById);
});
