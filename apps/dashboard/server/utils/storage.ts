import { promises as fsp } from 'node:fs';

import { nanoid } from 'nanoid';
import { basename, join } from 'pathe';

import { StorageType } from '#shared/prisma/enums';
import type { StorageModel } from '#shared/prisma/models';
import { createFtpDriver } from '~~/server/storage/ftp';
import { createLocalDriver } from '~~/server/storage/local';
import { createS3Driver } from '~~/server/storage/s3';
import { createSftpDriver } from '~~/server/storage/sftp';
import type { StorageDriver } from '~~/server/storage/types';

const localDriver = createLocalDriver(dataDirectory);
const drivers = new Map<string, { config: string; driver: StorageDriver }>();

export const storageMigration = { progress: null as { done: number; total: number } | null };

const driverFactories = {
    [StorageType.S3]: createS3Driver,
    [StorageType.SFTP]: createSftpDriver,
    [StorageType.FTP]: createFtpDriver,
};

export const uploadKey = (fileName: string) => `uploads/${fileName}`;

export const createStorageDriver = (type: StorageType, config: any): StorageDriver =>
    driverFactories[type](config);

export const getStorage = async (storageId: string | null) => {
    if (!storageId) return localDriver;

    const storage = await prisma.storage.findUniqueOrThrow({ where: { id: storageId } });
    const cached = drivers.get(storageId);

    if (cached?.config === storage.config) return cached.driver;

    await cached?.driver.close?.();

    const driver = createStorageDriver(storage.type, await readStorageConfig(storage));
    drivers.set(storageId, { config: storage.config, driver });

    return driver;
};

export const readStorageConfig = async (storage: Pick<StorageModel, 'config'>) =>
    JSON.parse(await decrypt(storage.config)) as Record<string, any>;

export const serializeStorage = async (storage: StorageModel) => {
    const config = await readStorageConfig(storage);
    const secrets = storageSecretKeys[storage.type];

    return {
        id: storage.id,
        name: storage.name,
        type: storage.type,
        createdAt: storage.createdAt,
        config: Object.fromEntries(
            Object.entries(config).map(([key, value]) => [key, secrets.includes(key) ? '' : value]),
        ),
        secrets: Object.fromEntries(secrets.map((key) => [key, !!config[key]])),
    };
};

export const withStoredSecrets = async (
    type: StorageType,
    config: Record<string, any>,
    storage?: Pick<StorageModel, 'config'> | null,
) => {
    const stored = storage ? await readStorageConfig(storage) : {};

    return {
        ...config,
        ...Object.fromEntries(
            storageSecretKeys[type].map((key) => [key, config[key] || stored[key] || '']),
        ),
    };
};

export const testStorage = async (type: StorageType, config: Record<string, any>) => {
    const driver = createStorageDriver(type, config);

    try {
        await driver.test();
    } catch (error: any) {
        throw apiError(400, 'errorsSystem.storageConnectFailed', { error: error.message });
    } finally {
        await driver.close?.();
    }
};

export const closeStorage = async (storageId: string) => {
    await drivers.get(storageId)?.driver.close?.();
    drivers.delete(storageId);
};

export const getActiveStorageId = async () => (await getSettings()).activeStorageId;

export const withLocalFile = async <T>(
    storageId: string | null,
    key: string,
    fn: (path: string) => Promise<T>,
) => {
    if (!storageId) return fn(join(dataDirectory, key));

    const dir = join(dataDirectory, 'temp', nanoid());
    const path = join(dir, basename(key));

    await fsp.mkdir(dir, { recursive: true });

    try {
        await (await getStorage(storageId)).download(key, path);
        return await fn(path);
    } finally {
        await fsp.rm(dir, { recursive: true, force: true });
    }
};
