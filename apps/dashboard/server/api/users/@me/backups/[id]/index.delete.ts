import { existsSync, promises as fsp } from 'node:fs';

import { join } from 'pathe';

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const backupId = getRouterParam(event, 'id');
    const backupPath = join(dataDirectory, 'backups', currentUser.id, `${backupId}.tgz`);

    if (!existsSync(backupPath)) {
        throw createError({
            statusCode: 404,
            message: 'Backup not found',
        });
    }

    await fsp.rm(backupPath, { force: true });

    await createLog(event, {
        action: 'Delete Backup',
        message: `Deleted backup ${backupId}`,
    });

    sendToUser(currentUser.id, 'backup:delete', backupId);
});
