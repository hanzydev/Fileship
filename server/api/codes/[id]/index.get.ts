import { sendToUser } from '~~/server/plugins/socketIO';

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
            author: {
                select: {
                    domains: true,
                },
            },
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

        await createLog(event, {
            action: 'View Code',
            message: `Viewed ${findCodeById.title}`,
            system: true,
        });

        if (
            findCodeById.maxViews &&
            findCodeById.views.length + 1 >= findCodeById.maxViews
        ) {
            await prisma.code.delete({
                where: {
                    id: findCodeById.id,
                },
            });

            await createLog(event, {
                action: 'Delete Code',
                message: `Deleted code ${findCodeById.title} due to max views reached`,
                system: true,
            });

            sendToUser(findCodeById.authorId, 'delete:code', findCodeById.id);
        }
    }

    const reqUrl = getRequestURL(event);

    const protocol = process.env.RETURN_HTTPS
        ? process.env.RETURN_HTTPS === 'true'
            ? 'https'
            : 'http'
        : reqUrl.protocol.slice(0, -1);

    const domain = findCodeById.author.domains.length
        ? findCodeById.author.domains[
              Math.floor(Math.random() * findCodeById.author.domains.length)
          ]
        : reqUrl.host;

    return {
        ...findCodeById,
        password: undefined,
        author: undefined,
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
        url: `${protocol}://${domain}/code/${findCodeById.id}`,
    };
});
