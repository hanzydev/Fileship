import { rm } from 'node:fs/promises';

import { join } from 'pathe';

import { sendByFilter, sendToUser } from '~~/server/plugins/socketIO';
import { isAdmin } from '~~/utils/user';

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;
    if (!currentUser) {
        throw createError({
            statusCode: 401,
            statusMessage: 'Unauthorized',
            message: 'You do not have permission to perform this action',
        });
    }

    const fileId = getRouterParam(event, 'id');

    const findFileById = await prisma.file.findUnique({
        where: {
            id: fileId,
        },
    });

    if (!findFileById) {
        throw createError({
            statusCode: 404,
            statusMessage: 'Not Found',
            message: 'File not found',
        });
    }

    if (findFileById.authorId !== currentUser.id) {
        throw createError({
            statusCode: 403,
            statusMessage: 'Forbidden',
            message: 'You do not have permission to perform this action',
        });
    }

    await rm(join(dataDirectory, 'uploads', findFileById.fileName)).catch(
        () => null,
    );

    await prisma.view.deleteMany({
        where: {
            fileId,
        },
    });

    await prisma.file.delete({
        where: {
            id: fileId,
        },
    });

    const log = await prisma.log.create({
        data: {
            action: 'Delete File',
            userId: currentUser.id,
            message: `Deleted ${findFileById.fileName}`,
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

    sendToUser(currentUser.id, 'delete:file', fileId);
    await sendByFilter(
        (socket) => isAdmin(socket.handshake.auth.user)!,
        'create:log',
        log,
    );
});
