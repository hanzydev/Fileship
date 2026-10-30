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
                        language: true,
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

    const access = findFileById && (await fileAccessOf(findFileById, currentUser?.id));
    const isOwner = !!access;

    if (!findFileById || (!isOwner && isHiddenFromVisitors(findFileById))) {
        throw apiError(404, 'errorsFiles.fileNotFound');
    }

    if (!isOwner) {
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

    const todayViews = await getTodayViews([findFileById.id]);
    const { domains } = findFileById.author;
    const directUrl = buildPublicUrl(event, domains, `/u/${findFileById.fileName}`);
    const embedUrl = buildPublicUrl(event, domains, `/view/${findFileById.fileName}`);
    const thumbnailUrl = getThumbnailUrl(event, domains, findFileById);
    const isVisual = findFileById.mimeType.startsWith('image/') || isVideoFile(findFileById as any);

    return {
        ...findFileById,
        _count: undefined,
        password: undefined,
        author: undefined,
        folder: undefined,
        ...(isOwner ? {} : visitorFileFields),
        access: access && access !== 'owner' ? access : undefined,
        size: {
            raw: findFileById.size.toString(),
            formatted: formatSize(findFileById.size),
        },
        views: {
            total: findFileById._count.views,
            today: todayViews.get(findFileById.id) ?? 0,
        },
        embed: embedOf(findFileById.author.embed),
        discordEmbed: discordEmbedFor(
            'file',
            findFileById.author,
            {
                name: findFileById.name,
                fileName: findFileById.fileName,
                mimeType: findFileById.mimeType,
                size: findFileById.size,
                url: embedUrl,
                fileUrl: isVisual ? directUrl : '',
                downloadUrl: `${directUrl}?download`,
                thumbnailUrl: thumbnailUrl ?? '',
            },
            findFileById.createdAt,
        ),
        directUrl,
        embedUrl,
        thumbnailUrl,
    };
});
