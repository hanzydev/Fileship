import { sendToUser } from '~~/server/plugins/socketIO';

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;

    const vanityOrId = getRouterParam(event, 'id');
    const query = getQuery(event);

    const findUrlById = await prisma.url.findFirst({
        where: {
            OR: [
                {
                    id: vanityOrId,
                },
                {
                    vanity: vanityOrId,
                },
            ],
        },
        include: {
            views: true,
            author: {
                select: {
                    domains: true,
                },
            },
        },
    });

    if (!findUrlById) {
        throw createError({
            statusCode: 404,
            statusMessage: 'Not Found',
            message: 'URL not found',
        });
    }

    if (findUrlById.authorId !== currentUser?.id && findUrlById.password) {
        if (!query.password) {
            throw createError({
                statusCode: 400,
                statusMessage: 'Bad Request',
                message: 'Verification is required',
            });
        }

        if (query.password !== findUrlById.password) {
            throw createError({
                statusCode: 401,
                statusMessage: 'Unauthorized',
                message: 'Invalid password',
            });
        }
    }

    if (query.log === 'true') {
        await prisma.view.create({
            data: {
                urlId: findUrlById.id,
                ip: getRequestIP(event, { xForwardedFor: true })!,
            },
        });

        await createLog(event, {
            action: 'View URL',
            message: `Viewed ${findUrlById.vanity}`,
            system: true,
        });

        if (
            findUrlById.maxViews &&
            findUrlById.views.length + 1 >= findUrlById.maxViews
        ) {
            await prisma.url.delete({
                where: {
                    id: findUrlById.id,
                },
            });

            await createLog(event, {
                action: 'Delete URL',
                message: `Deleted URL ${findUrlById.vanity} due to max views reached`,
                system: true,
            });

            sendToUser(findUrlById.authorId, 'delete:url', findUrlById.id);
        }
    }

    return {
        ...findUrlById,
        password: undefined,
        author: undefined,
        views: {
            total: findUrlById.views.length,
            today: findUrlById.views.filter((view) => {
                const now = new Date();

                return (
                    view.createdAt.getDate() === now.getDate() &&
                    view.createdAt.getMonth() === now.getMonth() &&
                    view.createdAt.getFullYear() === now.getFullYear()
                );
            }).length,
        },
        url: buildPublicUrl(
            event,
            findUrlById.author.domains,
            `/link/${findUrlById.vanity}`,
        ),
    };
});
