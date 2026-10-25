import { createReadStream, createWriteStream, promises as fsp } from 'node:fs';
import type { Readable } from 'node:stream';
import { pipeline } from 'node:stream/promises';

import {
    AbortMultipartUploadCommand,
    CompleteMultipartUploadCommand,
    CopyObjectCommand,
    CreateMultipartUploadCommand,
    DeleteObjectCommand,
    GetObjectCommand,
    HeadBucketCommand,
    HeadObjectCommand,
    ListObjectsV2Command,
    S3Client,
    UploadPartCommand,
    UploadPartCopyCommand,
} from '@aws-sdk/client-s3';
import { Upload } from '@aws-sdk/lib-storage';

import { notFound, type StorageDriver, type StoredObject } from './types';

const MAX_COPY_SIZE = 5 * 1024 ** 3;
const COPY_PART_SIZE = 512 * 1024 ** 2;
const COPY_CONCURRENCY = 4;

const trimTrailingSlashes = (value: string) => {
    let end = value.length;
    while (end > 0 && value[end - 1] === '/') end--;

    return value.slice(0, end);
};

export interface S3Config {
    endpoint?: string;
    region: string;
    bucket: string;
    prefix?: string;
    accessKeyId: string;
    secretAccessKey: string;
    forcePathStyle: boolean;
}

export const createS3Driver = (config: S3Config): StorageDriver => {
    const client = new S3Client({
        region: config.region,
        endpoint: config.endpoint || undefined,
        forcePathStyle: config.forcePathStyle,
        requestChecksumCalculation: 'WHEN_REQUIRED',
        responseChecksumValidation: 'WHEN_REQUIRED',
        credentials: {
            accessKeyId: config.accessKeyId,
            secretAccessKey: config.secretAccessKey,
        },
    });

    const prefix = config.prefix ? `${trimTrailingSlashes(config.prefix)}/` : '';
    const objectKey = (key: string) => `${prefix}${key}`;

    const getObject = async (key: string, range?: { start: number; end: number }) => {
        const { Body } = await client
            .send(
                new GetObjectCommand({
                    Bucket: config.bucket,
                    Key: objectKey(key),
                    Range: range && `bytes=${range.start}-${range.end}`,
                }),
            )
            .catch((error) => {
                if (error.name === 'NoSuchKey') throw notFound();
                throw error;
            });

        return Body as Readable;
    };

    const copyMultipart = async (copySource: string, to: string, size: number) => {
        const { UploadId } = await client.send(
            new CreateMultipartUploadCommand({ Bucket: config.bucket, Key: to }),
        );

        const total = Math.ceil(size / COPY_PART_SIZE);
        const parts: { PartNumber: number; ETag?: string }[] = [];
        let next = 1;

        const copyParts = async () => {
            while (next <= total) {
                const part = next++;
                const start = (part - 1) * COPY_PART_SIZE;
                const end = Math.min(start + COPY_PART_SIZE, size) - 1;

                const { CopyPartResult } = await client.send(
                    new UploadPartCopyCommand({
                        Bucket: config.bucket,
                        Key: to,
                        UploadId,
                        PartNumber: part,
                        CopySource: copySource,
                        CopySourceRange: `bytes=${start}-${end}`,
                    }),
                );

                parts.push({ PartNumber: part, ETag: CopyPartResult?.ETag });
            }
        };

        try {
            await Promise.all(Array.from({ length: COPY_CONCURRENCY }, copyParts));

            await client.send(
                new CompleteMultipartUploadCommand({
                    Bucket: config.bucket,
                    Key: to,
                    UploadId,
                    MultipartUpload: {
                        Parts: parts.sort((a, b) => a.PartNumber - b.PartNumber),
                    },
                }),
            );
        } catch (error) {
            await client
                .send(new AbortMultipartUploadCommand({ Bucket: config.bucket, Key: to, UploadId }))
                .catch(() => null);
            throw error;
        }
    };

    return {
        async put(key, localPath) {
            await new Upload({
                client,
                params: {
                    Bucket: config.bucket,
                    Key: objectKey(key),
                    Body: createReadStream(localPath),
                },
            }).done();

            await fsp.rm(localPath, { force: true });
        },
        read: getObject,
        async download(key, localPath) {
            await pipeline(await getObject(key), createWriteStream(localPath));
        },
        async move(from, to) {
            const copySource = `${config.bucket}/${objectKey(from).split('/').map(encodeURIComponent).join('/')}`;

            const { ContentLength } = await client
                .send(new HeadObjectCommand({ Bucket: config.bucket, Key: objectKey(from) }))
                .catch((error) => {
                    if (error.name === 'NotFound') throw notFound();
                    throw error;
                });

            if ((ContentLength ?? 0) > MAX_COPY_SIZE) {
                await copyMultipart(copySource, objectKey(to), ContentLength!);
            } else {
                await client.send(
                    new CopyObjectCommand({
                        Bucket: config.bucket,
                        CopySource: copySource,
                        Key: objectKey(to),
                    }),
                );
            }

            await client.send(
                new DeleteObjectCommand({ Bucket: config.bucket, Key: objectKey(from) }),
            );
        },
        async remove(key) {
            await client.send(
                new DeleteObjectCommand({ Bucket: config.bucket, Key: objectKey(key) }),
            );
        },
        async list(listPrefix) {
            const objects = new Map<string, StoredObject>();
            const base = objectKey(listPrefix);

            let token: string | undefined;

            do {
                const page = await client.send(
                    new ListObjectsV2Command({
                        Bucket: config.bucket,
                        Prefix: base,
                        ContinuationToken: token,
                    }),
                );

                for (const object of page.Contents ?? []) {
                    objects.set(object.Key!.slice(base.length), {
                        size: object.Size!,
                        modifiedAt: object.LastModified!,
                    });
                }

                token = page.NextContinuationToken;
            } while (token);

            return objects;
        },
        async test() {
            await client.send(new HeadBucketCommand({ Bucket: config.bucket }));
        },
        async beginUpload(key) {
            const { UploadId } = await client.send(
                new CreateMultipartUploadCommand({ Bucket: config.bucket, Key: objectKey(key) }),
            );

            return { uploadId: UploadId };
        },
        async writeChunk(key, state, part, _, data) {
            const { ETag } = await client.send(
                new UploadPartCommand({
                    Bucket: config.bucket,
                    Key: objectKey(key),
                    UploadId: state.uploadId,
                    PartNumber: part,
                    Body: data,
                }),
            );

            return { PartNumber: part, ETag };
        },
        async completeUpload(key, state, parts) {
            await client.send(
                new CompleteMultipartUploadCommand({
                    Bucket: config.bucket,
                    Key: objectKey(key),
                    UploadId: state.uploadId,
                    MultipartUpload: {
                        Parts: [...parts].sort((a, b) => a.PartNumber - b.PartNumber),
                    },
                }),
            );
        },
        async abortUpload(key, state) {
            await client.send(
                new AbortMultipartUploadCommand({
                    Bucket: config.bucket,
                    Key: objectKey(key),
                    UploadId: state.uploadId,
                }),
            );
        },
        async close() {
            client.destroy();
        },
    };
};
