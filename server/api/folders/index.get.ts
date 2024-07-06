export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;

    if (!currentUser) {
        throw createError({
            statusCode: 401,
            statusMessage: 'Unauthorized',
            message: 'You do not have permission to perform this action',
        });
    }

    const folders = await prisma.folder.findMany({
        where: {
            authorId: currentUser.id,
        },
        orderBy: {
            createdAt: 'desc',
        },
        include: {
            files: {
                select: {
                    id: true,
                },
            },
        },
    });

    return folders;
});
