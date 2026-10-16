import { createReadStream } from 'node:fs';

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;

    const fileNameOrId = decodeURIComponent(getRouterParam(event, 'id')!);
    const query = getQuery(event);

    let findFileById = null;

    for (const lookup of fileLookups(fileNameOrId)) {
        findFileById = await prisma.file.findFirst({
            where: lookup,
            include: {
                folder: {
                    select: {
                        public: true,
                    },
                },
            },
        });

        if (findFileById) break;
    }

    if (
        !findFileById ||
        (findFileById.authorId !== currentUser?.id &&
            (findFileById.deletedAt || isHiddenFromVisitors(findFileById)))
    ) {
        throw apiError(404, 'errorsFiles.fileNotFound');
    }

    if (findFileById.authorId !== currentUser?.id) {
        if (findFileById.folder?.public === false) {
            throw apiError(403, 'errorsFiles.noAccess');
        }

        if (
            findFileById.password &&
            !(await limitPasswordAttempts(
                event,
                findFileById.id,
                query.password === findFileById.password,
            ))
        ) {
            throw apiError(401, 'errorsFiles.invalidPassword');
        }
    }

    if (!canHaveThumbnail(findFileById.mimeType) || !hasThumbnail(findFileById.id)) {
        throw apiError(404, 'errorsFiles.thumbnailNotFound');
    }

    setResponseHeaders(event, {
        'Content-Type': 'image/jpeg',
        'Cache-Control': 'private, max-age=31536000, immutable',
    });

    return sendStream(event, createReadStream(getThumbnailPath(findFileById.id)));
});
