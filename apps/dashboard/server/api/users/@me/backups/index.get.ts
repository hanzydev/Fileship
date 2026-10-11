import { existsSync, promises as fsp } from 'node:fs';

import { filesize } from 'filesize';
import { basename, extname, join } from 'pathe';

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const query = await readListQuery(event);
    const userBackupsPath = join(dataDirectory, 'backups', currentUser.id);

    if (!existsSync(userBackupsPath)) return { items: [], total: 0 };

    const userBackups = await Promise.all(
        (await fsp.readdir(userBackupsPath))
            .filter((backup) => backup.endsWith('.tgz'))
            .map(async (backup) => {
                const backupStat = await fsp.stat(join(userBackupsPath, backup));

                return {
                    id: basename(backup, extname(backup)),
                    createdAt: backupStat.mtime,
                    size: {
                        raw: backupStat.size,
                        formatted: filesize(backupStat.size),
                    },
                };
            }),
    );

    return {
        items: userBackups
            .sort((a, b) => b.createdAt.getTime() - a.createdAt.getTime())
            .slice(query.offset, query.offset + query.limit),
        total: userBackups.length,
    };
});
