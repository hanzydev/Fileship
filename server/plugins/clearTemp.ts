import { existsSync, readdirSync, rmSync } from 'node:fs';

import { join } from 'pathe';

export default defineNitroPlugin(() => {
    const tempPath = join(dataDirectory, 'temp');
    if (existsSync(tempPath)) {
        const files = readdirSync(tempPath);

        for (const file of files) {
            rmSync(join(tempPath, file), { recursive: true, force: true });
        }
    }
});
