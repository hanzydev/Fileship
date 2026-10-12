import { promises as fsp } from 'node:fs';
import { PassThrough, Readable, Transform } from 'node:stream';

import { Client } from 'basic-ftp';
import { nanoid } from 'nanoid';
import { posix } from 'pathe';

import { notFound, type StorageDriver, type StoredObject } from './types';

export interface FtpConfig {
    host: string;
    port: number;
    user: string;
    password: string;
    secure: boolean;
    root: string;
}

const MAX_CONNECTIONS = 4;

export const createFtpDriver = (config: FtpConfig): StorageDriver => {
    let active = 0;
    const waiting: (() => void)[] = [];

    const open = async () => {
        if (active >= MAX_CONNECTIONS) await new Promise<void>((resolve) => waiting.push(resolve));
        active++;

        const client = new Client(30_000);
        let isReleased = false;

        const release = () => {
            if (isReleased) return;

            isReleased = true;
            client.close();
            active--;
            waiting.shift()?.();
        };

        try {
            await client.access({
                host: config.host,
                port: config.port,
                user: config.user,
                password: config.password,
                secure: config.secure,
            });
        } catch (error) {
            release();
            throw error;
        }

        return { client, release };
    };

    const withClient = async <T>(fn: (client: Client) => Promise<T>) => {
        const { client, release } = await open();

        try {
            return await fn(client);
        } finally {
            release();
        }
    };

    const path = (key: string) => posix.join(config.root, key);

    const exists = (client: Client, key: string) =>
        client.size(path(key)).then(
            () => true,
            () => false,
        );

    const list = async (
        client: Client,
        dir: string,
        base: string,
        objects: Map<string, StoredObject>,
    ) => {
        for (const entry of await client.list(dir)) {
            const entryPath = posix.join(dir, entry.name);

            if (entry.isDirectory) await list(client, entryPath, base, objects);
            else
                objects.set(posix.relative(base, entryPath), {
                    size: entry.size,
                    modifiedAt: entry.modifiedAt ?? new Date(0),
                });
        }

        return objects;
    };

    return {
        async put(key, localPath) {
            await withClient(async (client) => {
                await client.ensureDir(posix.dirname(path(key)));
                await client.uploadFrom(localPath, path(key));
            });

            await fsp.rm(localPath, { force: true });
        },
        async read(key, range) {
            const { client, release } = await open();

            if (!(await exists(client, key))) {
                release();
                throw notFound();
            }

            const output = new PassThrough();
            let remaining = range ? range.end - range.start + 1 : Infinity;

            const limit = new Transform({
                transform(chunk: Buffer, _, callback) {
                    const part = chunk.subarray(0, remaining);
                    remaining -= part.length;
                    callback(null, part);

                    if (remaining <= 0) {
                        this.end();
                        release();
                    }
                },
            });

            limit.pipe(output);
            output.on('close', release);

            client
                .downloadTo(limit, path(key), range?.start ?? 0)
                .then(() => limit.end())
                .catch((error) => remaining > 0 && output.destroy(error))
                .finally(release);

            return output;
        },
        async download(key, localPath) {
            await withClient((client) => client.downloadTo(localPath, path(key)));
        },
        async move(from, to) {
            await withClient(async (client) => {
                await client.ensureDir(posix.dirname(path(to)));
                await client.rename(path(from), path(to));
            });
        },
        async remove(key) {
            await withClient((client) => client.remove(path(key), true));
        },
        async list(prefix) {
            return withClient(async (client) => {
                const base = path(prefix);

                return (await client.cd(base).then(
                    () => true,
                    () => false,
                ))
                    ? list(client, base, base, new Map())
                    : new Map();
            });
        },
        async test() {
            await withClient((client) => client.ensureDir(config.root));
        },
        async beginUpload(key) {
            await withClient((client) => client.ensureDir(posix.dirname(path(key))));
            return { part: `${key}.${nanoid(8)}.part` };
        },
        async writeChunk(_, state, __, offset, data) {
            await withClient(async (client) => {
                const part = path(state.part);
                const source = Readable.from(Buffer.from(data));

                if (!offset) return client.uploadFrom(source, part);

                const size = await client.size(part).catch(() => 0);

                if (size >= offset + data.length) return;
                if (size !== offset) throw new Error('A previous upload chunk is missing');

                return client.appendFrom(source, part);
            });

            return state;
        },
        async completeUpload(key, state) {
            await withClient((client) => client.rename(path(state.part), path(key)));
        },
        async abortUpload(_, state) {
            await withClient((client) => client.remove(path(state.part), true));
        },
    };
};
