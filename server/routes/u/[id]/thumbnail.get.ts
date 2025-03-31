import { createReadStream, existsSync } from 'node:fs';

import { join } from 'pathe';

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

        if (findFileById.password && query.password !== findFileById.password) {
            throw createError({
                statusCode: 401,
                statusMessage: 'Unauthorized',
                message: 'Invalid password',
            });
        }
    }

    if (!findFileById.mimeType.startsWith('video/')) {
        throw createError({
            statusCode: 400,
            statusMessage: 'Bad Request',
            message: 'File is not a video',
        });
    }

    const thumbnailPath = join(dataDirectory, 'thumbnails', `${findFileById.id}.jpeg`);

    if (!existsSync(thumbnailPath)) {
        throw createError({
            statusCode: 404,
            statusMessage: 'Not Found',
            message: 'Thumbnail not found',
        });
    }

    return sendStream(event, createReadStream(thumbnailPath));
});
