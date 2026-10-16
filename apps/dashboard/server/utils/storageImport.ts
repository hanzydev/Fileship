import { promises as fsp } from 'node:fs';

import consola from 'consola';
import type { H3Event } from 'h3';
import mime from 'mime';
import { nanoid } from 'nanoid';
import { basename, join } from 'pathe';

import { insert } from '@orama/orama';

import type { User } from '~~/generated/prisma/client';

import type { StoredObject } from '../storage/types';

const SETTLE_TIME = 15 * 60 * 1000;
const IMPORT_DIRECTORY = 'import';
const FOLDER_NAME_LENGTH = 32;
const UPLOAD_PART = /\.[\w-]{8}\.part$/;

export interface StorageImportProgress {
    storageId: string | null;
    done: number;
    total: number;
    imported: number;
    skipped: number;
    failed: number;
}

export const storageImport = { progress: null as StorageImportProgress | null };

interface ImportableFile {
    key: string;
    size: number;
    modifiedAt: Date;
    name: string;
    folderName: string | null;
    isOrphanUpload: boolean;
}

export const localImportPath = async () => {
    const path = join(dataDirectory, IMPORT_DIRECTORY);
    await fsp.mkdir(path, { recursive: true });
    return path;
};

const removeEmptyDirectories = async (path: string, keep = false): Promise<boolean> => {
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
    const folderName =
        !isOrphanUpload && segments.length > 1
            ? toDisplayName(segments[0]!, '').slice(0, FOLDER_NAME_LENGTH) || null
            : null;

    return {
        key,
        size: object.size,
        modifiedAt: object.modifiedAt,
        name: toDisplayName(basename(key), 'file'),
        folderName,
        isOrphanUpload,
    };
};

export const findImportableFiles = async (storageId: string | null) => {
    const objects = storageId
        ? await listUnder(storageId, '')
        : [
              ...(await listUnder(null, 'uploads/')),
              ...(await listUnder(null, `${IMPORT_DIRECTORY}/`)),
          ];

    const settledBefore = Date.now() - SETTLE_TIME;
    const candidates = objects.filter(
        ([key, object]) =>
            !key.startsWith('backups/') &&
            !(key.startsWith('uploads/') && UPLOAD_PART.test(key)) &&
            !key.split('/').some((segment) => !segment || segment.startsWith('.')) &&
            object.modifiedAt.getTime() < settledBefore,
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

    return candidates
        .filter(([key]) => !key.startsWith('uploads/') || !known.has(key.slice('uploads/'.length)))
        .map(([key, object]) => toImportableFile(storageId, key, object));
};

const isFileNameTaken = async (fileName: string) =>
    !!(await prisma.file.findUnique({ where: { fileName }, select: { id: true } }));

const pickFileName = async (file: ImportableFile) => {
    const original = basename(file.key);

    if (file.isOrphanUpload && isValidFileName(original) && !(await isFileNameTaken(original)))
        return original;

    const extension = safeExtension(file.name);
    const base = (fileNameBase(file.name) || nanoid(8)).slice(0, 200);
    const fileName = `${base}${extension}`;

    return isValidFileName(fileName) && !(await isFileNameTaken(fileName))
        ? fileName
        : `${base}-${nanoid(6)}${extension}`;
};

export const runStorageImport = async (event: H3Event, storageId: string | null, owner: User) => {
    const report = (progress: StorageImportProgress | null) => {
        storageImport.progress = progress;
        sendByFilter((user) => !!user?.superAdmin, 'storage:import', progress);
    };

    const progress: StorageImportProgress = {
        storageId,
        done: 0,
        total: 0,
        imported: 0,
        skipped: 0,
        failed: 0,
    };

    report(progress);

    try {
        const files = await findImportableFiles(storageId);
        const storage = await getStorage(storageId);
        const folders = new Map<string, string>();

        progress.total = files.length;
        report({ ...progress });

        const findFolder = async (name: string) => {
            const cached = folders.get(name);
            if (cached) return cached;

            const existing = await prisma.folder.findFirst({
                where: { authorId: owner.id, name, deletedAt: null },
                select: { id: true },
            });

            if (existing) {
                folders.set(name, existing.id);
                return existing.id;
            }

            const created = await prisma.folder.create({
                data: { name, public: false, authorId: owner.id },
                include: folderInclude,
            });

            await insert(folderSearchDb, { id: created.id, authorId: owner.id, name });
            sendToUser(owner.id, 'folder:create', serializeFolder(event, created));
            folders.set(name, created.id);

            return created.id;
        };

        for (const file of files) {
            try {
                const isDuplicate = await prisma.file.findFirst({
                    where: {
                        authorId: owner.id,
                        name: file.name,
                        size: BigInt(file.size),
                        deletedAt: null,
                    },
                    select: { id: true },
                });

                if (isDuplicate || (await exceedsUsableSpace(owner, file.size))) {
                    progress.skipped++;
                    continue;
                }

                const fileName = await pickFileName(file);
                if (file.key !== uploadKey(fileName))
                    await storage.move(file.key, uploadKey(fileName));

                const created = await prisma.file.create({
                    data: {
                        fileName,
                        name: file.name,
                        size: BigInt(file.size),
                        storageId,
                        mimeType: mime.getType(file.name) ?? 'application/octet-stream',
                        authorId: owner.id,
                        folderId: file.folderName ? await findFolder(file.folderName) : null,
                        ...(file.modifiedAt.getTime() > 0 && { createdAt: file.modifiedAt }),
                    },
                    include: { _count: { select: { views: true } } },
                    omit: { embedding: true, textEmbedding: true, ocrText: true },
                });

                if (isAnalyzable(created.mimeType)) {
                    await withLocalFile(storageId, uploadKey(fileName), (path) =>
                        processMedia(created, path),
                    )
                        .then((metadata) => (created.metadata = metadata as any))
                        .catch((error) =>
                            consola.warn(`Could not process ${fileName}: ${error.message}`),
                        );
                }

                await indexCreatedFile(event, owner, created);
                sendToUser(owner.id, 'file:create', serializeFile(event, created, owner.domains));

                progress.imported++;
            } catch (error: any) {
                progress.failed++;
                consola.warn(`Could not import ${file.key}: ${error.message}`);
            } finally {
                progress.done++;
                report({ ...progress });
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
        report(null);
    }

    return progress;
};
