import { existsSync, promises as fsp } from 'node:fs';

import consola from 'consola';
import dayjs from 'dayjs';
import { join } from 'pathe';
import { titleCase } from 'scule';

export default defineTask({
    meta: {
        name: 'dir:createDirectories',
        description: 'Creates directories',
    },
    async run() {
        const directories = ['uploads', 'backups', 'temp', 'thumbnails'];

        await Promise.all(
            directories.map(async (directory) => {
                const path = join(dataDirectory, directory);
                if (!existsSync(path)) {
                    await fsp.mkdir(path, { recursive: true });
                    consola.info(
                        `${dayjs().format('YYYY-MM-DD HH:mm:ss')} - ${titleCase(directory)} folder created.`,
                    );
                }
            }),
        );

        return { result: 'success' };
    },
});
