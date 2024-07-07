import { existsSync, promises as fsp } from 'node:fs';

import { join } from 'pathe';

import { sendToUser } from '~~/server/plugins/socketIO';

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

    await fsp.rm(backupPath, { force: true });

    await createLog(event, {
        action: 'Delete Backup',
        message: `Deleted ${backupId}`,
    });

    sendToUser(currentUser.id, 'delete:backup', backupId);
});
