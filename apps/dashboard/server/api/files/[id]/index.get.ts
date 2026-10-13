import { defu } from 'defu';
import { filesize } from 'filesize';

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;

    const fileNameOrId = getRouterParam(event, 'id');
    const query = getQuery(event);

    const findFileById = await prisma.file.findFirst({
        where: {
            deletedAt: null,
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
            _count: { select: { views: true } },
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
        omit: {
            embedding: true,
            textEmbedding: true,
            ocrText: true,
        },
    });

    if (!findFileById) {
        throw createError({
            status: 404,
            message: 'File not found',
        });
    }

    if (findFileById.authorId !== currentUser?.id) {
        if (findFileById.folder?.public === false) {
            throw createError({
                status: 403,
                message: 'You do not have permission to access this page',
            });
        }

        if (findFileById.password) {
            if (!query.password) {
                throw createError({
                    status: 400,
                    message: 'Verification is required',
                });
            }

            await rateLimit(
                `filePassword:${getRequestIP(event, { xForwardedFor: true })}:${findFileById.id}`,
                20,
                10 * 60 * 1000,
            );

            if (query.password !== findFileById.password) {
                throw createError({
                    status: 401,
                    message: 'Invalid password',
                });
            }
        }
    }

    const isOwner = findFileById.authorId === currentUser?.id;
    const todayViews = await getTodayViews([findFileById.id]);

    return {
        ...findFileById,
        _count: undefined,
        password: undefined,
        author: undefined,
        folder: undefined,
        ...(isOwner
            ? {}
            : {
                  authorId: undefined,
                  caption: undefined,
                  piiReasons: undefined,
                  piiOverride: undefined,
                  maxViews: undefined,
                  expiresAt: undefined,
              }),
        size: {
            raw: findFileById.size.toString(),
            formatted: filesize(findFileById.size),
        },
        views: {
            total: findFileById._count.views,
            today: todayViews.get(findFileById.id) ?? 0,
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
        thumbnailUrl: getThumbnailUrl(event, findFileById.author.domains, findFileById),
    };
});
