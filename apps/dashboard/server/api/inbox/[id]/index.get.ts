export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;

    const inboxSlugOrId = getRouterParam(event, 'id');
    const query = getQuery(event);

    const findInboxById = await prisma.inbox.findFirst({
        where: {
            OR: [
                {
                    id: inboxSlugOrId,
                },
                {
                    slug: inboxSlugOrId,
                },
            ],
        },
        include: {
            user: {
                select: {
                    id: true,
                    username: true,
                    avatar: true,
                },
            },
        },
    });

    if (!findInboxById?.enabled) {
        throw createError({
            statusCode: 404,
            message: 'Inbox not found',
        });
    }

    if (findInboxById.userId !== currentUser?.id && findInboxById.password) {
        if (!query.password) {
            throw createError({
                statusCode: 400,
                message: 'Verification is required',
            });
        }

        if (query.password !== findInboxById.password) {
            throw createError({
                statusCode: 401,
                message: 'Invalid password',
            });
        }
    }

    return {
        id: findInboxById.id,
        user: findInboxById.user,
    };
});
