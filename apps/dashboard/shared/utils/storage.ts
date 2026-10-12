import { z } from 'zod';

import { StorageType } from '#shared/prisma/enums';

const host = z.string().trim().min(1, 'Host is required');
const port = z.number().int().min(1).max(65535);
const root = z.string().trim().min(1, 'Root folder is required');

export const storageConfigSchemas = {
    [StorageType.S3]: z.object({
        endpoint: z.url().or(z.literal('')),
        region: z.string().trim().min(1, 'Region is required'),
        bucket: z.string().trim().min(1, 'Bucket is required'),
        prefix: z.string().trim(),
        accessKeyId: z.string().trim().min(1, 'Access key is required'),
        secretAccessKey: z.string(),
        forcePathStyle: z.boolean(),
    }),
    [StorageType.SFTP]: z.object({
        host,
        port,
        username: z.string().trim().min(1, 'Username is required'),
        password: z.string(),
        privateKey: z.string(),
        root,
    }),
    [StorageType.FTP]: z.object({
        host,
        port,
        user: z.string().trim().min(1, 'Username is required'),
        password: z.string(),
        secure: z.boolean(),
        root,
    }),
};

export const storageSecretKeys: Record<StorageType, string[]> = {
    [StorageType.S3]: ['secretAccessKey'],
    [StorageType.SFTP]: ['password', 'privateKey'],
    [StorageType.FTP]: ['password'],
};

export const storageInputSchema = z.discriminatedUnion('type', [
    z.object({ type: z.literal(StorageType.S3), config: storageConfigSchemas.S3 }),
    z.object({ type: z.literal(StorageType.SFTP), config: storageConfigSchemas.SFTP }),
    z.object({ type: z.literal(StorageType.FTP), config: storageConfigSchemas.FTP }),
]);
