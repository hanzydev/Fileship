import { existsSync, readdirSync, rmSync } from 'node:fs';

import consola from 'consola';
import dayjs from 'dayjs';
import { join } from 'pathe';

export default defineTask({
    meta: {
        name: 'dir:clearTemp',
        description: 'Clears temporary directory',
    },
    run() {
        const tempPath = join(dataDirectory, 'temp');

        if (existsSync(tempPath)) {
            const files = readdirSync(tempPath);

            consola.info(`${dayjs().format('YYYY-MM-DD HH:mm:ss')} - Clearing temp folder`);

            for (const file of files) {
                rmSync(join(tempPath, file), { recursive: true, force: true });
            }
        }

        return { result: 'success' };
    },
});
