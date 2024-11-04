import { defu } from 'defu';
import { filesize } from 'filesize';

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;

    const fileNameOrId = getRouterParam(event, 'id');
    const query = getQuery(event);

    const findFileById = await prisma.file.findFirst({
        where: {
            OR: [
                {
                    id: fileNameOrId,
                },
                {
                    fileName: fileNameOrId,
                },
            ],
        },
        include: {
            views: true,
            folder: {
                select: {
                    public: true,
                },
            },
            author: {
                select: {
                    embed: true,
                    domains: true,
                },
            },
        },
    });

    if (!findFileById) {
        throw createError({
            statusCode: 404,
            statusMessage: 'Not Found',
            message: 'File not found',
        });
    }

    if (findFileById.authorId !== currentUser?.id) {
        if (findFileById.folder?.public === false) {
            throw createError({
                statusCode: 403,
                statusMessage: 'Forbidden',
                message: 'You do not have permission to access this page',
            });
        }

        if (findFileById.password) {
            if (!query.password) {
                throw createError({
                    statusCode: 400,
                    statusMessage: 'Bad Request',
                    message: 'Verification is required',
                });
            }

            if (query.password !== findFileById.password) {
                throw createError({
                    statusCode: 401,
                    statusMessage: 'Unauthorized',
                    message: 'Invalid password',
                });
            }
        }
    }

    return {
        ...findFileById,
        password: undefined,
        author: undefined,
        folder: undefined,
        size: {
            raw: findFileById.size.toString(),
            formatted: filesize(findFileById.size.toString()),
        },
        views: {
            total: findFileById.views.length,
            today: findFileById.views.filter((view) => {
                const now = new Date();

                return (
                    view.createdAt.getDate() === now.getDate() &&
                    view.createdAt.getMonth() === now.getMonth() &&
                    view.createdAt.getFullYear() === now.getFullYear()
                );
            }).length,
        },
        embed: defu(findFileById.author.embed, defaultEmbed) as IEmbed,
        directUrl: buildPublicUrl(
            event,
            findFileById.author.domains,
            `/u/${findFileById.fileName}`,
        ),
        embedUrl: buildPublicUrl(
            event,
            findFileById.author.domains,
            `/view/${findFileById.fileName}`,
        ),
    };
});
