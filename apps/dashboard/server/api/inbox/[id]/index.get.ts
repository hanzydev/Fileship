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
                    permissions: true,
                    superAdmin: true,
                },
            },
        },
    });

    if (!findInboxById?.enabled || !canUploadFiles(findInboxById.user)) {
        throw createError({
            status: 404,
            message: 'Inbox not found',
        });
    }

    if (findInboxById.userId !== currentUser?.id && findInboxById.password) {
        if (!query.password) {
            throw createError({
                status: 400,
                message: 'Verification is required',
            });
        }

        if (
            !(await limitPasswordAttempts(
                event,
                findInboxById.id,
                query.password === findInboxById.password,
            ))
        ) {
            throw createError({
                status: 401,
                message: 'Invalid password',
            });
        }
    }

    return {
        id: findInboxById.id,
        user: {
            id: findInboxById.user.id,
            username: findInboxById.user.username,
            avatar: findInboxById.user.avatar,
        },
    };
});
