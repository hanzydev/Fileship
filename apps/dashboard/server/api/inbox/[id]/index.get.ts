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
        throw apiError(404, 'errorsContent.inboxNotFound');
    }

    if (findInboxById.userId !== currentUser?.id && findInboxById.password) {
        if (!query.password) {
            throw apiError(400, 'errorsContent.passwordRequired');
        }

        if (
            !(await limitPasswordAttempts(
                event,
                findInboxById.id,
                query.password === findInboxById.password,
            ))
        ) {
            throw apiError(401, 'errorsContent.invalidPassword');
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
