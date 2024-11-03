import { existsSync, mkdirSync } from 'node:fs';

import consola from 'consola';
import dayjs from 'dayjs';
import { join } from 'pathe';

export default defineNitroPlugin(() => {
    const directories = ['uploads', 'backups', 'temp'];

    directories.forEach((directory) => {
        const path = join(dataDirectory, directory);
        if (!existsSync(path)) {
            consola.info(`${dayjs().format('YYYY-MM-DD HH:mm:ss')} - Creating ${directory} folder`);

            mkdirSync(path, { recursive: true });
        }
    });
});
