import { existsSync, mkdirSync } from 'node:fs';

import { join } from 'pathe';

export default defineNitroPlugin(() => {
    const directories = ['uploads', 'backups', 'temp'];

    directories.forEach((directory) => {
        const path = join(dataDirectory, directory);
        if (!existsSync(path)) mkdirSync(path, { recursive: true });
    });
});
