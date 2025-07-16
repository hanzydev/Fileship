import { createReadStream, promises as fsp } from 'node:fs';

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
            _count: {
                select: {
                    views: true,
                },
            },
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
            message: 'File not found',
        });
    }

    if (findFileById.authorId !== currentUser?.id) {
        if (findFileById.folder?.public === false) {
            throw createError({
                statusCode: 403,
                message: 'You do not have permission to access this page',
            });
        }

        if (findFileById.password) {
            if (!query.password) {
                return await sendRedirect(event, `/view/${fileNameOrId}`);
            }

            if (query.password !== findFileById.password) {
                throw createError({
                    statusCode: 401,
                    message: 'Invalid password',
                });
            }
        }
    }

    const filePath = join(dataDirectory, 'uploads', findFileById.fileName);

    if (findFileById.authorId !== currentUser?.id) {
        await prisma.view.create({
            data: {
                fileId: findFileById.id,
                ip: getRequestIP(event, { xForwardedFor: true })!,
            },
        });

        await createLog(event, {
            action: 'View File',
            message: `Viewed ${findFileById.fileName}`,
            system: true,
        });

        if (findFileById.maxViews && findFileById._count.views >= findFileById.maxViews) {
            await fsp.rm(filePath, { force: true });

            await prisma.file.delete({
                where: {
                    id: findFileById.id,
                },
            });

            await createLog(event, {
                action: 'Delete File',
                message: `Deleted file ${findFileById.fileName} due to max views reached`,
                system: true,
            });

            sendToUser(findFileById.authorId, 'delete:file', findFileById.id);

            throw createError({
                statusCode: 404,
                message: 'File not found',
            });
        }
    }

    setResponseHeaders(event, {
        'Content-Type': findFileById.mimeType,
        'Content-Length': findFileById.size.toString(),
        'Accept-Ranges': 'bytes',
    });

    if ('download' in query) {
        setResponseHeader(
            event,
            'Content-Disposition',
            `attachment; filename="${findFileById.fileName}"`,
        );
    }

    return sendStream(event, createReadStream(filePath));
});
