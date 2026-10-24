import { createHash } from 'node:crypto';

import type { H3Event } from 'h3';

type HashableFile = { id: string; authorId: string; fileName: string; storageId: string | null };

type StoredFile = HashableFile & { name: string; contentHash: string | null };

export const hashableFileSelect = {
    id: true,
    name: true,
    authorId: true,
    fileName: true,
    storageId: true,
    contentHash: true,
} as const;

const hashing = new Map<string, Promise<string | null>>();
const sync = { running: null as Promise<void> | null, isQueued: false };

const readContentHash = async (file: HashableFile) => {
    const hash = createHash('sha256');
    const stream = await (await getStorage(file.storageId)).read(uploadKey(file.fileName));

    for await (const chunk of stream) hash.update(chunk);

    return hash.digest('hex');
};

const storeContentHash = async (file: HashableFile) => {
    const contentHash = await readContentHash(file);

    await prisma.file.updateMany({ where: { id: file.id }, data: { contentHash } });

    const copies = await prisma.file.count({
        where: { authorId: file.authorId, contentHash, deletedAt: null, id: { not: file.id } },
    });

    if (copies) sendToUser(file.authorId, 'duplicates:update', null);

    return contentHash;
};

export const hashFile = (file: HashableFile) => {
    if (!hashing.has(file.id)) {
        hashing.set(
            file.id,
            storeContentHash(file)
                .catch(() => null)
                .finally(() => hashing.delete(file.id)),
        );
    }

    return hashing.get(file.id)!;
};

const contentHashOf = (file: StoredFile) => file.contentHash ?? hashFile(file);

const findSameSize = (file: { id?: string; authorId: string; size: bigint }) =>
    prisma.file.findMany({
        where: {
            authorId: file.authorId,
            size: file.size,
            deletedAt: null,
            ...(file.id && { id: { not: file.id } }),
        },
        select: hashableFileSelect,
        orderBy: { createdAt: 'asc' },
    });

const findByHash = async (files: StoredFile[], contentHash: string) => {
    for (const file of files) {
        if ((await contentHashOf(file)) === contentHash) return file;
    }

    return null;
};

export const reportDuplicate = async (file: HashableFile & { name: string; size: bigint }) => {
    if (!file.size) return;

    const copies = await findSameSize(file);
    if (!copies.length) return;

    const contentHash = await hashFile(file);
    const original = contentHash && (await findByHash(copies, contentHash));

    if (original)
        sendToUser(file.authorId, 'file:duplicate', {
            id: file.id,
            name: file.name,
            original: { id: original.id, name: original.name },
        });
};

export const checkUploads = async (
    event: H3Event,
    authorId: string,
    uploads: { size: number; hash?: string }[],
) => {
    const sizes = [...new Set(uploads.map((upload) => upload.size).filter(Boolean))];
    const files = await prisma.file.findMany({
        where: { authorId, deletedAt: null, size: { in: sizes.map(BigInt) } },
        select: { ...hashableFileSelect, size: true },
        orderBy: { createdAt: 'asc' },
    });
    const bySize = Map.groupBy(files, (file) => Number(file.size));
    const results = [];

    for (const upload of uploads) {
        const copies = bySize.get(upload.size) ?? [];

        if (copies.length && !upload.hash) {
            event.waitUntil(
                (async () => {
                    for (const copy of copies) await contentHashOf(copy);
                })(),
            );
        }

        const original = copies.length && upload.hash && (await findByHash(copies, upload.hash));

        results.push({
            needsHash: !!copies.length && !upload.hash,
            original: original ? { id: original.id, name: original.name } : null,
        });
    }

    return results;
};

const hashPendingFiles = async () => {
    let cursor = '';

    while (true) {
        const files = await prisma.$queryRaw<HashableFile[]>`
            SELECT f."id", f."authorId", f."fileName", f."storageId"
            FROM "File" f
            WHERE f."contentHash" IS NULL AND f."deletedAt" IS NULL AND f."size" > 0
                AND f."id" > ${cursor}
                AND EXISTS (
                    SELECT 1 FROM "File" g
                    WHERE g."authorId" = f."authorId" AND g."size" = f."size"
                        AND g."id" <> f."id" AND g."deletedAt" IS NULL
                )
            ORDER BY f."id"
            LIMIT 100
        `;

        if (!files.length) return;

        for (const file of files) await hashFile(file);

        cursor = files.at(-1)!.id;
    }
};

const runSync = async () => {
    do {
        sync.isQueued = false;
        await hashPendingFiles().catch(() => null);
    } while (sync.isQueued);

    sync.running = null;
};

export const syncContentHashes = () => {
    if (sync.running) sync.isQueued = true;
    else sync.running = runSync();

    return sync.running;
};

export const findDuplicateGroups = async (authorId: string, offset: number, limit: number) => {
    const groups = await prisma.$queryRaw<{ contentHash: string; size: bigint; count: number }[]>`
        SELECT "contentHash", "size", COUNT(*)::int AS "count"
        FROM "File"
        WHERE "authorId" = ${authorId} AND "deletedAt" IS NULL AND "contentHash" IS NOT NULL
        GROUP BY "contentHash", "size"
        HAVING COUNT(*) > 1
        ORDER BY "size" * (COUNT(*) - 1) DESC, "contentHash"
        OFFSET ${offset}
        LIMIT ${limit}
    `;

    const [summary] = await prisma.$queryRaw<{ groups: number; files: number; wasted: bigint }[]>`
        SELECT COUNT(*)::int AS "groups", COALESCE(SUM("count"), 0)::int AS "files",
            COALESCE(SUM("size" * ("count" - 1)), 0)::bigint AS "wasted"
        FROM (
            SELECT "size", COUNT(*) AS "count"
            FROM "File"
            WHERE "authorId" = ${authorId} AND "deletedAt" IS NULL AND "contentHash" IS NOT NULL
            GROUP BY "contentHash", "size"
            HAVING COUNT(*) > 1
        ) AS "duplicates"
    `;

    return { groups, summary: summary! };
};
