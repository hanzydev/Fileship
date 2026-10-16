import { defu } from 'defu';

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;

    const fileNameOrId = getRouterParam(event, 'id')!;
    const query = getQuery(event);

    let findFileById = null;

    for (const lookup of fileLookups(fileNameOrId)) {
        findFileById = await prisma.file.findFirst({
            where: {
                ...lookup,
                deletedAt: null,
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

        if (findFileById) break;
    }

    if (
        !findFileById ||
        (findFileById.authorId !== currentUser?.id && isHiddenFromVisitors(findFileById))
    ) {
        throw apiError(404, 'errorsFiles.fileNotFound');
    }

    if (findFileById.authorId !== currentUser?.id) {
        if (findFileById.folder?.public === false) {
            throw apiError(403, 'errorsFiles.noAccess');
        }

        if (findFileById.password) {
            if (!query.password) {
                throw apiError(400, 'errorsFiles.verificationRequired');
            }

            if (
                !(await limitPasswordAttempts(
                    event,
                    findFileById.id,
                    query.password === findFileById.password,
                ))
            ) {
                throw apiError(401, 'errorsFiles.invalidPassword');
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
        ...(isOwner ? {} : visitorFileFields),
        size: {
            raw: findFileById.size.toString(),
            formatted: formatSize(findFileById.size),
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
