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

            for (const file of files) {
                rmSync(join(tempPath, file), { recursive: true, force: true });
            }

            consola.success(`${dayjs().format('YYYY-MM-DD HH:mm:ss')} - Temp folder cleared.`);
        }

        return { result: 'success' };
    },
});
