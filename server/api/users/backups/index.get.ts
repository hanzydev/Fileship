import { existsSync, promises as fsp } from 'node:fs';

import { filesize } from 'filesize';
import { basename, extname, join } from 'pathe';

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;

    if (!currentUser) {
        throw createError({
            statusCode: 401,
            statusMessage: 'Unauthorized',
            message: 'You do not have permission to perform this action',
        });
    }

    const userBackupsPath = join(dataDirectory, 'backups', currentUser.id);
    if (!existsSync(userBackupsPath)) return [];

    const userBackups = await fsp.readdir(userBackupsPath);

    return Promise.all(
        userBackups.map(async (backup) => {
            const backupStat = await fsp.stat(join(userBackupsPath, backup));

            return {
                id: basename(backup, extname(backup)),
                createdAt: backupStat.birthtime,
                size: {
                    raw: backupStat.size,
                    formatted: filesize(backupStat.size),
                },
            };
        }),
    );
});
