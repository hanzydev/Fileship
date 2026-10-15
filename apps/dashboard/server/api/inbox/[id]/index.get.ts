export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;

    const inboxSlugOrId = getRouterParam(event, 'id');
    const query = getQuery(event);

    const include = {
        user: {
            select: {
                id: true,
                username: true,
                avatar: true,
                permissions: true,
                superAdmin: true,
            },
        },
    } as const;

    const findInboxById =
        (isUuid(inboxSlugOrId!)
            ? await prisma.inbox.findUnique({ where: { id: inboxSlugOrId }, include })
            : null) ?? (await prisma.inbox.findUnique({ where: { slug: inboxSlugOrId }, include }));

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
