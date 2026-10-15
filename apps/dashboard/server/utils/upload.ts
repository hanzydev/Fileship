import { randomUUID } from 'node:crypto';
import { promises as fsp } from 'node:fs';

import { basename, dirname, extname, join } from 'pathe';
import sharp from 'sharp';

import type { UploadPart, UploadState } from '~~/server/storage/types';

interface UploadSession {
    kind: string;
    key: string;
    storageId: string | null;
    isDirect: boolean;
    total: number;
    chunkSize: number | null;
    received: number;
    parts: Record<number, { size: number; attempt: string; isDone: boolean; result?: UploadPart }>;
    isComplete?: boolean;
    state: UploadState;
}

interface UploadChunk {
    kind: string;
    uploadId: string;
    part: number;
    total: number;
    offset: number;
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

const sessionLocks = new Map<string, Promise<unknown>>();

const withSessionLock = <T>(path: string, fn: () => Promise<T>) => {
    const result = (sessionLocks.get(path) ?? Promise.resolve()).then(fn, fn);
    const tail = result.catch(() => null);

    sessionLocks.set(path, tail);
    tail.then(() => {
        if (sessionLocks.get(path) === tail) sessionLocks.delete(path);
    });

    return result;
};

const readSession = (path: string): Promise<UploadSession | null> =>
    fsp.readFile(`${path}.json`, 'utf8').then(JSON.parse, () => null);

const invalidChunk = (message: string) => createError({ status: 400, message });

const checkLayout = (session: UploadSession, chunk: UploadChunk) => {
    const isLast = chunk.part === chunk.total;

    if (chunk.total !== session.total || chunk.part > session.total)
        throw invalidChunk('Invalid upload chunk');

    if (session.total === 1) {
        if (chunk.offset) throw invalidChunk('Invalid upload chunk');
        return;
    }

    const chunkSize = isLast ? chunk.offset / (session.total - 1) : chunk.data.length;

    if (!Number.isInteger(chunkSize) || chunkSize < 1) throw invalidChunk('Invalid upload chunk');

    session.chunkSize ??= chunkSize;

    if (
        chunkSize !== session.chunkSize ||
        chunk.offset !== (chunk.part - 1) * session.chunkSize ||
        (isLast && chunk.data.length > session.chunkSize)
    )
        throw invalidChunk('Invalid upload chunk');
};

export const receiveChunk = async (
    ownerId: string,
    chunk: UploadChunk,
    start: () => Promise<{ key: string; isDirect: boolean }>,
    isTooLarge: (size: number) => Promise<boolean> = async () => false,
) => {
    const path = join(dataDirectory, 'temp', ownerId, `${chunk.uploadId}.upload`);
    const attempt = randomUUID();

    const session = await withSessionLock(path, async () => {
        let session = await readSession(path);

        if (!session) {
            const { key, isDirect } = await start();
            const storageId = await getActiveStorageId();

            await fsp.mkdir(dirname(path), { recursive: true });
            if (!isDirect) await fsp.writeFile(path, new Uint8Array());

            session = {
                kind: chunk.kind,
                key,
                storageId,
                isDirect,
                total: chunk.total,
                chunkSize: null,
                received: 0,
                parts: {},
                state: isDirect ? await (await getStorage(storageId)).beginUpload(key) : {},
            };
        }

        if (session.kind !== chunk.kind) throw invalidChunk('Upload not found');
        if (session.isComplete) throw invalidChunk('Upload already completed');

        checkLayout(session, chunk);

        const received =
            session.received - (session.parts[chunk.part]?.size ?? 0) + chunk.data.length;

        if (await isTooLarge(received)) {
            await fsp.writeFile(`${path}.json`, JSON.stringify(session));
            await abortUploadSession(path);
            throw createError({
                status: 400,
                message: 'Storage limit reached',
            });
        }

        session.received = received;
        session.parts[chunk.part] = { size: chunk.data.length, attempt, isDone: false };

        await fsp.writeFile(`${path}.json`, JSON.stringify(session));
        return session;
    });

    const result = await (
        session.isDirect
            ? (await getStorage(session.storageId)).writeChunk(
                  session.key,
                  session.state,
                  chunk.part,
                  chunk.offset,
                  chunk.data,
              )
            : writeUploadChunk(path, chunk.data, chunk.offset, chunk.part === chunk.total)
    ).catch(async (error) => {
        await withSessionLock(path, async () => {
            const current = await readSession(path);
            if (current?.parts[chunk.part]?.attempt !== attempt) return;

            current.received -= chunk.data.length;
            current.parts = Object.fromEntries(
                Object.entries(current.parts).filter(([part]) => Number(part) !== chunk.part),
            );
            await fsp.writeFile(`${path}.json`, JSON.stringify(current));
        });

        throw error;
    });

    return withSessionLock(path, async () => {
        const session = await readSession(path);

        if (!session || session.isComplete) throw invalidChunk('Upload not found');

        const entry = session.parts[chunk.part];
        if (entry?.attempt !== attempt) return;

        entry.isDone = true;
        if (result) entry.result = result;

        session.isComplete =
            Object.values(session.parts).filter((part) => part.isDone).length === session.total;

        await fsp.writeFile(`${path}.json`, JSON.stringify(session));

        if (!session.isComplete) return;

        const parts = Object.values(session.parts).flatMap((part) =>
            part.result ? [part.result] : [],
        );

        return { path, session, parts };
    });
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

    const { path, session, parts } = upload;
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
            await storage.completeUpload(session.key, session.state, parts);
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
