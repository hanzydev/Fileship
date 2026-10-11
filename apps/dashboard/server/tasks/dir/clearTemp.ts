import { existsSync, promises as fsp } from 'node:fs';

import consola from 'consola';
import dayjs from 'dayjs';
import { join } from 'pathe';

const TEMP_MAX_AGE = 6 * 60 * 60 * 1000;

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
                    const { mtimeMs } = await fsp.stat(join(tempPath, file));
                    if (Date.now() - mtimeMs < TEMP_MAX_AGE) continue;

                    await fsp.rm(join(tempPath, file), { recursive: true, force: true });
                } catch {
                    //
                }
            }

            consola.success(`${dayjs().format('YYYY-MM-DD HH:mm:ss')} - Old temp files cleared.`);
        }

        return { result: 'success' };
    },
});
