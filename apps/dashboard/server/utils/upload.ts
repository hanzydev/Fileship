import { promises as fsp } from 'node:fs';

import { basename, dirname, extname, join } from 'pathe';
import sharp from 'sharp';

import type { UploadState } from '~~/server/storage/types';

interface UploadSession {
    key: string;
    storageId: string | null;
    isDirect: boolean;
    received: number;
    state: UploadState;
}

interface UploadChunk {
    uploadId: string;
    part: number;
    offset: number;
    isLast: boolean;
    data: Uint8Array;
}

type UploadOwner = NonNullable<PartialUser> & { id: string; limits: any };

interface UploadedFile {
    id: string;
    fileName: string;
    mimeType: string;
    storageId: string | null;
    metadata: any;
}

export const isProcessableImage = (mimeType: string) =>
    mimeType.startsWith('image/') && !['image/gif', 'image/svg+xml'].includes(mimeType);

export const exceedsUsableSpace = async (owner: UploadOwner, size: number) => {
    if (isAdmin(owner) || owner.limits.usableSpace < 0) return false;

    const { _sum } = await prisma.file.aggregate({
        where: { authorId: owner.id },
        _sum: { size: true },
    });

    return (_sum.size ?? 0n) + BigInt(size) > BigInt(owner.limits.usableSpace * 1024 * 1024);
};

export const abortUploadSession = async (path: string) => {
    const session: UploadSession = JSON.parse(await fsp.readFile(`${path}.json`, 'utf8'));

    await fsp.rm(`${path}.json`, { force: true });
    await fsp.rm(path, { force: true });

    if (session.isDirect)
        await (
            await getStorage(session.storageId)
        )
            .abortUpload(session.key, session.state)
            .catch(() => null);
};

export const receiveChunk = async (
    ownerId: string,
    chunk: UploadChunk,
    start: () => Promise<{ key: string; isDirect: boolean }>,
    isTooLarge: (size: number) => Promise<boolean> = async () => false,
) => {
    const path = join(dataDirectory, 'temp', ownerId, `${chunk.uploadId}.upload`);

    let session: UploadSession | null = await fsp
        .readFile(`${path}.json`, 'utf8')
        .then(JSON.parse, () => null);

    if (!session) {
        if (chunk.offset) {
            throw createError({
                status: 400,
                message: 'Upload not found',
            });
        }

        const { key, isDirect } = await start();
        const storageId = await getActiveStorageId();

        session = {
            key,
            storageId,
            isDirect,
            received: 0,
            state: isDirect ? await (await getStorage(storageId)).beginUpload(key) : {},
        };

        await fsp.mkdir(dirname(path), { recursive: true });
        await fsp.writeFile(`${path}.json`, JSON.stringify(session));
    }

    if (chunk.offset > session.received) {
        throw createError({
            status: 400,
            message: 'A previous upload chunk is missing',
        });
    }

    const received = Math.max(session.received, chunk.offset + chunk.data.length);

    if (await isTooLarge(received)) {
        await abortUploadSession(path);
        throw createError({
            status: 400,
            message: 'Storage limit reached',
        });
    }

    if (session.isDirect)
        session.state = await (
            await getStorage(session.storageId)
        ).writeChunk(session.key, session.state, chunk.part, chunk.offset, chunk.data);
    else await writeUploadChunk(path, chunk.data, chunk.offset, chunk.isLast);

    session.received = received;
    await fsp.writeFile(`${path}.json`, JSON.stringify(session));

    if (chunk.isLast) return { path, session };
};

export const receiveUpload = async <T extends UploadedFile>(
    owner: UploadOwner,
    chunk: UploadChunk & { mimeType: string; compression?: number },
    getFileName: () => Promise<string>,
    create: (data: { fileName: string; size: number; storageId: string | null }) => Promise<T>,
) => {
    const { removeExifData } = await getSettings();

    const upload = await receiveChunk(
        owner.id,
        chunk,
        async () => ({
            key: uploadKey(await getFileName()),
            isDirect: !(
                isProcessableImage(chunk.mimeType) &&
                (removeExifData || !!chunk.compression)
            ),
        }),
        (size) => exceedsUsableSpace(owner, size),
    );

    if (!upload) return;

    const { path, session } = upload;
    const storage = await getStorage(session.storageId);

    if (!session.isDirect) {
        const processedPath = `${path}${extname(session.key)}`;

        try {
            let image = sharp(path).rotate();

            if (chunk.compression) {
                image = image.jpeg({
                    quality: Math.min(100, Math.max(1, 100 - chunk.compression)),
                });
            }

            if (!removeExifData) image = image.keepExif();

            await image.toFile(processedPath);
            await fsp.rename(processedPath, path);
        } catch {
            //
        }

        session.received = (await fsp.stat(path)).size;

        if (await exceedsUsableSpace(owner, session.received)) {
            await abortUploadSession(path);
            throw createError({
                status: 400,
                message: 'Storage limit reached',
            });
        }
    }

    const file = await create({
        fileName: basename(session.key),
        size: session.received,
        storageId: session.storageId,
    }).catch(async (error) => {
        await abortUploadSession(path);
        throw error;
    });

    try {
        if (session.isDirect) {
            await storage.completeUpload(session.key, session.state);
        } else {
            if (isAnalyzable(file.mimeType)) file.metadata = await processMedia(file, path);
            await storage.put(session.key, path);
        }
    } catch (error) {
        await prisma.file.delete({ where: { id: file.id } });
        await removeFileData([file]);
        await abortUploadSession(path);
        throw error;
    }

    await fsp.rm(`${path}.json`, { force: true });

    if (session.isDirect && isAnalyzable(file.mimeType))
        file.metadata = await withLocalFile(session.storageId, session.key, (filePath) =>
            processMedia(file, filePath),
        ).catch(() => null);

    return file;
};
