import { promises as fsp } from 'node:fs';

import { dirname, extname, join } from 'pathe';
import sharp from 'sharp';

import type { UploadState } from '~~/server/storage/types';

interface UploadSession {
    fileName: string;
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
    mimeType: string;
    compression?: number;
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
            .abortUpload(uploadKey(session.fileName), session.state)
            .catch(() => null);
};

export const receiveUpload = async <T extends UploadedFile>(
    owner: UploadOwner,
    chunk: UploadChunk,
    getFileName: () => Promise<string>,
    create: (data: { fileName: string; size: number; storageId: string | null }) => Promise<T>,
) => {
    const path = join(dataDirectory, 'temp', owner.id, `${chunk.uploadId}.upload`);
    const { removeExifData } = await getSettings();

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

        const fileName = await getFileName();
        const storageId = await getActiveStorageId();
        const isDirect = !(
            isProcessableImage(chunk.mimeType) &&
            (removeExifData || !!chunk.compression)
        );

        session = {
            fileName,
            storageId,
            isDirect,
            received: 0,
            state: isDirect
                ? await (await getStorage(storageId)).beginUpload(uploadKey(fileName))
                : {},
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

    if (await exceedsUsableSpace(owner, received)) {
        await abortUploadSession(path);
        throw createError({
            status: 400,
            message: 'Storage limit reached',
        });
    }

    const storage = await getStorage(session.storageId);
    const key = uploadKey(session.fileName);

    if (session.isDirect)
        session.state = await storage.writeChunk(
            key,
            session.state,
            chunk.part,
            chunk.offset,
            chunk.data,
        );
    else await writeUploadChunk(path, chunk.data, chunk.offset, chunk.isLast);

    session.received = received;
    await fsp.writeFile(`${path}.json`, JSON.stringify(session));

    if (!chunk.isLast) return;

    if (!session.isDirect) {
        const processedPath = `${path}${extname(session.fileName)}`;

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
        fileName: session.fileName,
        size: session.received,
        storageId: session.storageId,
    }).catch(async (error) => {
        await abortUploadSession(path);
        throw error;
    });

    try {
        if (session.isDirect) {
            await storage.completeUpload(key, session.state);
        } else {
            if (isAnalyzable(file.mimeType)) file.metadata = await processMedia(file, path);
            await storage.put(key, path);
        }
    } catch (error) {
        await prisma.file.delete({ where: { id: file.id } });
        await removeFileData([file]);
        await abortUploadSession(path);
        throw error;
    }

    await fsp.rm(`${path}.json`, { force: true });

    if (session.isDirect && isAnalyzable(file.mimeType))
        file.metadata = await withLocalFile(session.storageId, key, (filePath) =>
            processMedia(file, filePath),
        ).catch(() => null);

    return file;
};
