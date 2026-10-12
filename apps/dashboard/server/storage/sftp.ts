import { promises as fsp } from 'node:fs';

import { posix } from 'pathe';
import SftpClient from 'ssh2-sftp-client';

import { notFound, type StorageDriver, type StoredObject } from './types';

export interface SftpConfig {
    host: string;
    port: number;
    username: string;
    password?: string;
    privateKey?: string;
    root: string;
}

export const createSftpDriver = (config: SftpConfig): StorageDriver => {
    let connection: Promise<SftpClient> | undefined;

    const connect = () =>
        (connection ??= (async () => {
            const client = new SftpClient();

            client.on('end', () => (connection = undefined));
            client.on('error', () => (connection = undefined));

            await client.connect({
                host: config.host,
                port: config.port,
                username: config.username,
                password: config.password || undefined,
                privateKey: config.privateKey || undefined,
            });

            return client;
        })().catch((error) => {
            connection = undefined;
            throw error;
        }));

    const path = (key: string) => posix.join(config.root, key);

    const list = async (
        client: SftpClient,
        dir: string,
        base: string,
        objects: Map<string, StoredObject>,
    ) => {
        for (const entry of await client.list(dir)) {
            const entryPath = posix.join(dir, entry.name);

            if (entry.type === 'd') await list(client, entryPath, base, objects);
            else
                objects.set(posix.relative(base, entryPath), {
                    size: entry.size,
                    modifiedAt: new Date(entry.modifyTime),
                });
        }

        return objects;
    };

    return {
        async put(key, localPath) {
            const client = await connect();

            await client.mkdir(posix.dirname(path(key)), true);
            await client.fastPut(localPath, path(key));
            await fsp.rm(localPath, { force: true });
        },
        async read(key, range) {
            const client = await connect();

            if (!(await client.exists(path(key)))) throw notFound();

            return client.createReadStream(path(key), range);
        },
        async download(key, localPath) {
            await (await connect()).fastGet(path(key), localPath);
        },
        async move(from, to) {
            const client = await connect();

            await client.mkdir(posix.dirname(path(to)), true);
            await client.rename(path(from), path(to));
        },
        async remove(key) {
            await (await connect()).delete(path(key), true);
        },
        async list(prefix) {
            const client = await connect();
            const base = path(prefix);

            if (!(await client.exists(base))) return new Map();

            return list(client, base, base, new Map());
        },
        async test() {
            const client = await connect();

            await client.mkdir(config.root, true);
        },
        async close() {
            const client = await connection?.catch(() => null);

            connection = undefined;
            await client?.end();
        },
    };
};
