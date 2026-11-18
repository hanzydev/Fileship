import { readdirSync } from 'node:fs';
import { dirname, join, resolve, sep } from 'node:path';

export const root = resolve(import.meta.dirname, '..');
export const native = join(root, 'native');
export const generated = join(root, 'build', 'generated');
export const platform = process.platform === 'win32' ? 'windows' : 'linux';

const PLATFORMS = ['windows', 'linux'];

export const groupSources = (folders, isIncluded = () => true) => {
    const groups = {};

    for (const folder of folders) {
        for (const file of readdirSync(join(native, folder), { recursive: true })) {
            const path = join(folder, file);
            const parts = dirname(path).split(sep);

            if (!file.endsWith('.cpp') || !isIncluded(path)) continue;
            if (parts.some((part) => PLATFORMS.includes(part) && part !== platform)) continue;

            (groups[parts.join('-')] ??= []).push(join(native, path));
        }
    }

    return groups;
};
