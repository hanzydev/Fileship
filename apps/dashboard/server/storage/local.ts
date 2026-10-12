import { createReadStream, promises as fsp } from 'node:fs';

import { dirname, join, relative } from 'pathe';

import { notFound, type StorageDriver, type StoredObject } from './types';

export const createLocalDriver = (root: string): StorageDriver => {
    const path = (key: string) => join(root, key);

    return {
        async put(key, localPath) {
            await fsp.mkdir(dirname(path(key)), { recursive: true });
            await moveFileRobust(localPath, path(key));
        },
        async read(key, range) {
            await fsp.stat(path(key)).catch(() => {
                throw notFound();
            });

            return createReadStream(path(key), range);
        },
        async download(key, localPath) {
            await fsp.copyFile(path(key), localPath);
        },
        async move(from, to) {
            await fsp.mkdir(dirname(path(to)), { recursive: true });
            await moveFileRobust(path(from), path(to));
        },
        async remove(key) {
            await fsp.rm(path(key), { force: true });
        },
        async list(prefix) {
            const objects = new Map<string, StoredObject>();
            const base = path(prefix);
            const entries = await fsp.readdir(base, { recursive: true, withFileTypes: true });

            for (const entry of entries.filter((entry) => entry.isFile())) {
                const filePath = join(entry.parentPath, entry.name);
                const stat = await fsp.stat(filePath);

                objects.set(relative(base, filePath), { size: stat.size, modifiedAt: stat.mtime });
            }

            return objects;
        },
        async test() {
            await fsp.access(root);
        },
    };
};
