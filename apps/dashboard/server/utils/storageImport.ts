import { promises as fsp } from 'node:fs';

import consola from 'consola';
import type { H3Event } from 'h3';
import mime from 'mime';
import { nanoid } from 'nanoid';
import { basename, join } from 'pathe';

import { insert } from '@orama/orama';

import type { ImportApp } from '#shared/utils/importApps';
import type { User } from '~~/generated/prisma/client';

import type { StoredObject } from '../storage/types';

const SETTLE_TIME = 15 * 60 * 1000;
const IMPORT_DIRECTORY = 'import';
const FOLDER_NAME_LENGTH = 32;
const UPLOAD_PART = /\.[\w-]{8}\.part$/;

export interface StorageImportProgress {
    storageId: string | null;
    app: ImportApp | null;
    done: number;
    total: number;
    imported: number;
    skipped: number;
    missing: number;
    failed: number;
}

export const storageImport = { progress: null as StorageImportProgress | null };

interface ImportableFile {
    key: string;
    size: number;
    modifiedAt: Date;
    name: string;
    folderPath: string[];
    isOrphanUpload: boolean;
}

export const localImportPath = async () => {
    const path = join(dataDirectory, IMPORT_DIRECTORY);
    await fsp.mkdir(path, { recursive: true });
    return path;
};

export const toFolderName = (name: string) =>
    toDisplayName(name, '').slice(0, FOLDER_NAME_LENGTH).trim();

export const removeEmptyDirectories = async (path: string, keep = false): Promise<boolean> => {
    const entries = await fsp.readdir(path, { withFileTypes: true }).catch(() => []);
    const removed = await Promise.all(
        entries.map((entry) =>
            entry.isDirectory() ? removeEmptyDirectories(join(path, entry.name)) : false,
        ),
    );

    if (keep || removed.some((isRemoved) => !isRemoved)) return false;

    return fsp.rmdir(path).then(
        () => true,
        () => false,
    );
};

const listUnder = async (storageId: string | null, prefix: string) => {
    const objects = await (await getStorage(storageId)).list(prefix).catch((error) => {
        if (storageId) throw error;
        return new Map<string, StoredObject>();
    });

    return [...objects].map(([key, object]) => [`${prefix}${key}`, object] as const);
};

const withoutFileshipObjects = async (objects: (readonly [string, StoredObject])[]) => {
    const candidates = objects.filter(
        ([key]) =>
            !key.startsWith('backups/') && !(key.startsWith('uploads/') && UPLOAD_PART.test(key)),
    );
    const uploadNames = candidates
        .filter(([key]) => key.startsWith('uploads/'))
        .map(([key]) => key.slice('uploads/'.length));
    const known = new Set<string>();

    for (let index = 0; index < uploadNames.length; index += 1000) {
        const files = await prisma.file.findMany({
            where: { fileName: { in: uploadNames.slice(index, index + 1000) } },
            select: { fileName: true },
        });
        files.forEach((file) => known.add(file.fileName));
    }

    return candidates.filter(
        ([key]) => !key.startsWith('uploads/') || !known.has(key.slice('uploads/'.length)),
    );
};

export const listImportObjects = async (storageId: string | null) =>
    withoutFileshipObjects(
        storageId ? await listUnder(storageId, '') : await listUnder(null, `${IMPORT_DIRECTORY}/`),
    );

const toImportableFile = (
    storageId: string | null,
    key: string,
    object: StoredObject,
): ImportableFile => {
    const path =
        !storageId && key.startsWith(`${IMPORT_DIRECTORY}/`)
            ? key.slice(IMPORT_DIRECTORY.length + 1)
            : key;
    const segments = path.split('/');
    const isOrphanUpload = key.startsWith('uploads/') && segments.length === 2;
    const folderPath = isOrphanUpload
        ? []
        : segments.slice(0, -1).map(toFolderName).filter(Boolean).slice(0, MAX_FOLDER_DEPTH);

    return {
        key,
        size: object.size,
        modifiedAt: object.modifiedAt,
        name: toDisplayName(basename(key), 'file'),
        folderPath,
        isOrphanUpload,
    };
};

export const findImportableFiles = async (storageId: string | null) => {
    const objects = await withoutFileshipObjects(
        storageId
            ? await listUnder(storageId, '')
            : [
                  ...(await listUnder(null, 'uploads/')),
                  ...(await listUnder(null, `${IMPORT_DIRECTORY}/`)),
              ],
    );

    const settledBefore = Date.now() - SETTLE_TIME;

    return objects
        .filter(
            ([key, object]) =>
                !key.split('/').some((segment) => !segment || segment.startsWith('.')) &&
                object.modifiedAt.getTime() < settledBefore,
        )
        .map(([key, object]) => toImportableFile(storageId, key, object));
};

const isFileNameTaken = async (fileName: string) =>
    !!(await prisma.file.findUnique({ where: { fileName }, select: { id: true } }));

export const availableFileName = async (name: string, preferred?: string) => {
    if (preferred && isValidFileName(preferred) && !(await isFileNameTaken(preferred)))
        return preferred;

    const extension = safeExtension(name);
    const base = (fileNameBase(name) || nanoid(8)).slice(0, 200);
    const fileName = `${base}${extension}`;

    return isValidFileName(fileName) && !(await isFileNameTaken(fileName))
        ? fileName
        : `${base}-${nanoid(6)}${extension}`;
};

export const reportImport = (progress: StorageImportProgress | null) => {
    storageImport.progress = progress && { ...progress };
    sendByFilter((user) => !!user?.superAdmin, 'storage:import', storageImport.progress);
};

export const createFolderResolver = (event: H3Event, owner: User) => {
    const folders = new Map<string, string>();

    const findFolder = async (path: string[], isPublic = false): Promise<string | null> => {
        if (!path.length) return null;

        const key = path.join('/');
        const cached = folders.get(key);
        if (cached) return cached;

        const name = path.at(-1)!;
        const parentId = await findFolder(path.slice(0, -1));
        const existing = await prisma.folder.findFirst({
            where: { authorId: owner.id, name, parentId, deletedAt: null },
            select: { id: true },
        });

        if (existing) {
            folders.set(key, existing.id);
            return existing.id;
        }

        const created = await prisma.folder.create({
            data: { name, public: isPublic, authorId: owner.id, parentId },
            include: folderInclude,
        });

        await insert(folderSearchDb, { id: created.id, authorId: owner.id, name });
        sendToUser(owner.id, 'folder:create', serializeFolder(event, created));
        folders.set(key, created.id);

        return created.id;
    };

    return findFolder;
};

export const isDuplicateImport = async (owner: User, name: string, size: number) =>
    !!(await prisma.file.findFirst({
        where: { authorId: owner.id, name, size: BigInt(size), deletedAt: null },
        select: { id: true },
    }));

export const importStoredFile = async (
    event: H3Event,
    owner: User,
    storageId: string | null,
    file: {
        key: string;
        fileName: string;
        name: string;
        size: number;
        folderId: string | null;
        createdAt: Date;
        mimeType?: string | null;
        maxViews?: number;
        expiresAt?: Date | null;
        password?: string | null;
        views?: number;
    },
) => {
    const key = uploadKey(file.fileName);
    const views = file.views ?? 0;

    if (file.key !== key) await (await getStorage(storageId)).move(file.key, key);

    const created = await prisma.file.create({
        data: {
            fileName: file.fileName,
            name: file.name,
            size: BigInt(file.size),
            storageId,
            mimeType:
                (file.mimeType !== 'application/octet-stream' && file.mimeType) ||
                mime.getType(file.name) ||
                'application/octet-stream',
            authorId: owner.id,
            folderId: file.folderId,
            maxViews: file.maxViews,
            expiresAt: file.expiresAt,
            password: file.password,
            ...(file.createdAt.getTime() > 0 && { createdAt: file.createdAt }),
        },
        include: { _count: { select: { views: true } } },
        omit: { embedding: true, textEmbedding: true, ocrText: true },
    });

    for (let index = 0; index < views; index += 10_000) {
        await prisma.view.createMany({
            data: Array.from({ length: Math.min(10_000, views - index) }, () => ({
                fileId: created.id,
                ownerId: owner.id,
                ip: '',
                createdAt: created.createdAt,
            })),
        });
    }

    created._count.views = views;

    if (isAnalyzable(created.mimeType)) {
        await withLocalFile(storageId, key, (path) => processMedia(created, path))
            .then((metadata) => (created.metadata = metadata as any))
            .catch((error) => consola.warn(`Could not process ${file.fileName}: ${error.message}`));
    }

    await indexCreatedFile(event, owner, created);
    sendToUser(owner.id, 'file:create', serializeFile(event, created, owner.domains));

    return created;
};

export const runStorageImport = async (event: H3Event, storageId: string | null, owner: User) => {
    const progress: StorageImportProgress = {
        storageId,
        app: null,
        done: 0,
        total: 0,
        imported: 0,
        skipped: 0,
        missing: 0,
        failed: 0,
    };

    reportImport(progress);

    try {
        const files = await findImportableFiles(storageId);
        const findFolder = createFolderResolver(event, owner);

        progress.total = files.length;
        reportImport(progress);

        for (const file of files) {
            try {
                if (
                    (await isDuplicateImport(owner, file.name, file.size)) ||
                    (await exceedsUsableSpace(owner, file.size))
                ) {
                    progress.skipped++;
                    continue;
                }

                await importStoredFile(event, owner, storageId, {
                    key: file.key,
                    fileName: await availableFileName(
                        file.name,
                        file.isOrphanUpload ? basename(file.key) : undefined,
                    ),
                    name: file.name,
                    size: file.size,
                    folderId: await findFolder(file.folderPath),
                    createdAt: file.modifiedAt,
                });

                progress.imported++;
            } catch (error: any) {
                progress.failed++;
                consola.warn(`Could not import ${file.key}: ${error.message}`);
            } finally {
                progress.done++;
                reportImport(progress);
            }
        }

        if (!storageId) await removeEmptyDirectories(await localImportPath(), true);

        await createLog(event, {
            action: 'Import Files',
            message: `Imported ${progress.imported} of ${progress.total} files from ${
                storageId ? `storage ${storageId}` : 'the data directory'
            } for ${owner.username}: ${progress.skipped} skipped, ${progress.failed} failed`,
        });
    } catch (error: any) {
        consola.error(`Storage import failed: ${error.message}`);
    } finally {
        reportImport(null);
    }

    return progress;
};
