import { sendByFilter, sendToUser } from '~~/server/plugins/socketIO';
import { isAdmin } from '~~/utils/user';

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;

    const codeId = getRouterParam(event, 'id');
    const query = getQuery(event);

    const findCodeById = await prisma.code.findUnique({
        where: {
            id: codeId,
        },
        include: {
            views: true,
        },
    });

    if (!findCodeById) {
        throw createError({
            statusCode: 404,
            statusMessage: 'Not Found',
            message: 'Code not found',
        });
    }

    if (findCodeById.authorId !== currentUser?.id && findCodeById.password) {
        if (!query.password) {
            throw createError({
                statusCode: 400,
                statusMessage: 'Bad Request',
                message: 'Verification is required',
            });
        }

        if (query.password !== findCodeById.password) {
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
                codeId: findCodeById.id,
                ip: getRequestIP(event, { xForwardedFor: true })!,
            },
        });

        const createLog = await prisma.log.create({
            data: {
                action: 'View Code',
                message: `Viewed ${findCodeById.title}`,
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
            findCodeById.maxViews &&
            findCodeById.maxViews <= findCodeById.views.length
        ) {
            await prisma.code.delete({
                where: {
                    id: findCodeById.id,
                },
            });

            const deleteLog = await prisma.log.create({
                data: {
                    action: 'Delete Code',
                    message: `Deleted ${findCodeById.title} due to max views reached`,
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

            sendToUser(findCodeById.authorId, 'delete:code', findCodeById.id);
        }
    }

    return {
        ...findCodeById,
        password: undefined,
        views: {
            total: findCodeById.views.length,
            today: findCodeById.views.filter((view) => {
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
