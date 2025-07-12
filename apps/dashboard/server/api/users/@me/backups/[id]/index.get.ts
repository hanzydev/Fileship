import { createReadStream, existsSync, promises as fsp } from 'node:fs';

import { join } from 'pathe';

export default defineEventHandler(async (event) => {
    userOnly(event);

    const backupId = getRouterParam(event, 'id');
    const backupPath = join(dataDirectory, 'backups', event.context.user!.id, `${backupId}.tgz`);

    if (!existsSync(backupPath)) {
        throw createError({
            statusCode: 404,
            statusMessage: 'Not Found',
            message: 'Backup not found',
        });
    }

    const backupStat = await fsp.stat(backupPath);

    setResponseHeaders(event, {
        'Content-Type': 'application/gzip',
        'Content-Length': backupStat.size.toString(),
        'Accept-Ranges': 'bytes',
    });

    setResponseHeader(event, 'Content-Disposition', `attachment; filename="${backupId}.tgz"`);

    return sendStream(event, createReadStream(backupPath));
});
