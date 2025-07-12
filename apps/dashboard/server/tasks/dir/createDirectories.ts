import { existsSync, mkdirSync } from 'node:fs';

import consola from 'consola';
import dayjs from 'dayjs';
import { join } from 'pathe';
import { titleCase } from 'scule';

export default defineTask({
    meta: {
        name: 'dir:createDirectories',
        description: 'Creates directories',
    },
    run() {
        const directories = ['uploads', 'backups', 'temp', 'thumbnails'];

        directories.forEach((directory) => {
            const path = join(dataDirectory, directory);
            if (!existsSync(path)) {
                mkdirSync(path, { recursive: true });
                consola.info(
                    `${dayjs().format('YYYY-MM-DD HH:mm:ss')} - ${titleCase(directory)} folder created.`,
                );
            }
        });

        return { result: 'success' };
    },
});
