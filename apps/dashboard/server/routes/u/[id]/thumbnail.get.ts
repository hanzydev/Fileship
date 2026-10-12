import { createReadStream } from 'node:fs';

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;

    const fileNameOrId = decodeURIComponent(getRouterParam(event, 'id')!);
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
            folder: {
                select: {
                    public: true,
                },
            },
        },
    });

    if (!findFileById || (findFileById.deletedAt && findFileById.authorId !== currentUser?.id)) {
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

        if (findFileById.password && query.password !== findFileById.password) {
            throw createError({
                status: 401,
                message: 'Invalid password',
            });
        }
    }

    if (!canHaveThumbnail(findFileById.mimeType) || !hasThumbnail(findFileById.id)) {
        throw createError({
            status: 404,
            message: 'Thumbnail not found',
        });
    }

    setResponseHeaders(event, {
        'Content-Type': 'image/jpeg',
        'Cache-Control': 'private, max-age=31536000, immutable',
    });

    return sendStream(event, createReadStream(getThumbnailPath(findFileById.id)));
});
