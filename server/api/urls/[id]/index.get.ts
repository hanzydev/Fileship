import { sendByFilter, sendToUser } from '~~/server/plugins/socketIO';
import { isAdmin } from '~~/utils/user';

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

        const createLog = await prisma.log.create({
            data: {
                action: 'View URL',
                message: `Viewed ${findUrlById.vanity}`,
                system: true,
                ip: getRequestIP(event, { xForwardedFor: true })!,
            },
            include: {
                user: {
                    select: {
                        id: true,
                        username: true,
                    },
                },
            },
        });

        await sendByFilter(
            (socket) => isAdmin(socket.handshake.auth.user)!,
            'create:log',
            createLog,
        );

        if (
            findUrlById.maxViews &&
            findUrlById.maxViews <= findUrlById.views.length
        ) {
            await prisma.url.delete({
                where: {
                    id: findUrlById.id,
                },
            });

            const deleteLog = await prisma.log.create({
                data: {
                    action: 'Delete URL',
                    message: `Deleted ${findUrlById.vanity} due to max views reached`,
                    system: true,
                    ip: getRequestIP(event, { xForwardedFor: true })!,
                },
                include: {
                    user: {
                        select: {
                            id: true,
                            username: true,
                        },
                    },
                },
            });

            await sendByFilter(
                (socket) => isAdmin(socket.handshake.auth.user)!,
                'create:log',
                deleteLog,
            );

            sendToUser(findUrlById.authorId, 'delete:url', findUrlById.id);
        }
    }

    return {
        ...findUrlById,
        password: undefined,
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
    };
});
