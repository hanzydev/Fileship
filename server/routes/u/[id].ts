import { createReadStream, promises as fsp } from 'node:fs';

import { join } from 'pathe';

import { sendByFilter, sendToUser } from '~~/server/plugins/socketIO';
import { isAdmin } from '~~/utils/user';

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
            views: true,
            author: true,
        },
    });

    if (!findFileById) {
        throw createError({
            statusCode: 404,
            statusMessage: 'Not Found',
            message: 'File not found',
        });
    }

    if (findFileById.authorId !== currentUser?.id && findFileById.password) {
        if (!query.password) {
            return await sendRedirect(event, `/view/${fileNameOrId}`);
        }

        if (query.password !== findFileById.password) {
            throw createError({
                statusCode: 401,
                statusMessage: 'Unauthorized',
                message: 'Invalid password',
            });
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

        const createLog = await prisma.log.create({
            data: {
                action: 'View File',
                message: `Viewed ${findFileById.fileName}`,
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
            findFileById.maxViews &&
            findFileById.maxViews > findFileById.views.length
        ) {
            await fsp.rm(filePath);

            await prisma.file.delete({
                where: {
                    id: findFileById.id,
                },
            });

            const deleteLog = await prisma.log.create({
                data: {
                    action: 'Delete File',
                    message: `Deleted ${findFileById.fileName} due to max views reached`,
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

            sendToUser(findFileById.authorId, 'delete:file', findFileById.id);

            throw createError({
                statusCode: 404,
                statusMessage: 'Not Found',
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
