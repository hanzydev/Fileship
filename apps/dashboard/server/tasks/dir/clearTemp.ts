import { existsSync, promises as fsp } from 'node:fs';

import consola from 'consola';
import dayjs from 'dayjs';
import { join } from 'pathe';

export default defineTask({
    meta: {
        name: 'dir:clearTemp',
        description: 'Clears temporary directory',
    },
    async run() {
        const tempPath = join(dataDirectory, 'temp');

        if (existsSync(tempPath)) {
            const files = await fsp.readdir(tempPath);

            for (const file of files) {
                try {
                    await fsp.rm(join(tempPath, file), { recursive: true, force: true });
                } catch {
                    //
                }
            }

            consola.success(`${dayjs().format('YYYY-MM-DD HH:mm:ss')} - Temp folder cleared.`);
        }

        return { result: 'success' };
    },
});
