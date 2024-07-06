import { existsSync, promises as fsp } from 'node:fs';

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

    const backupId = getRouterParam(event, 'id');
    const backupPath = join(
        dataDirectory,
        'backups',
        currentUser.id,
        `${backupId}.tgz`,
    );

    if (!existsSync(backupPath)) {
        throw createError({
            statusCode: 404,
            statusMessage: 'Not Found',
            message: 'Backup not found',
        });
    }

    await fsp.rm(backupPath);

    const log = await prisma.log.create({
        data: {
            action: 'Delete Backup',
            userId: currentUser.id,
            message: `Deleted ${backupId}`,
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

    sendToUser(currentUser.id, 'delete:backup', backupId);
    await sendByFilter(
        (socket) => isAdmin(socket.handshake.auth.user)!,
        'create:log',
        log,
    );
});
