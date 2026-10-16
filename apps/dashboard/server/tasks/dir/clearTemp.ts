import { existsSync, promises as fsp } from 'node:fs';

import consola from 'consola';
import dayjs from 'dayjs';
import { join } from 'pathe';

const TEMP_MAX_AGE = 6 * 60 * 60 * 1000;

const isStale = async (path: string) => Date.now() - (await fsp.stat(path)).mtimeMs > TEMP_MAX_AGE;

export default defineTask({
    meta: {
        name: 'dir:clearTemp',
        description: 'Clears old temporary files',
    },
    async run() {
        const tempPath = join(dataDirectory, 'temp');

        if (existsSync(tempPath)) {
            for (const entry of await fsp.readdir(tempPath, { withFileTypes: true })) {
                const entryPath = join(tempPath, entry.name);

                try {
                    if (entry.isDirectory()) {
                        for (const child of await fsp.readdir(entryPath)) {
                            const childPath = join(entryPath, child);
                            if (!(await isStale(childPath))) continue;

                            if (child.endsWith('.upload.json'))
                                await abortUploadSession(childPath.slice(0, -'.json'.length));
                            else await fsp.rm(childPath, { recursive: true, force: true });
                        }

                        if (!(await fsp.readdir(entryPath)).length && (await isStale(entryPath))) {
                            await fsp.rm(entryPath, { recursive: true, force: true });
                        }
                    } else if (await isStale(entryPath)) {
                        await fsp.rm(entryPath, { force: true });
                    }
                } catch {
                    //
                }
            }

            consola.success(`${dayjs().format('YYYY-MM-DD HH:mm:ss')} - Old temp files cleared.`);
        }

        return { result: 'success' };
    },
});
