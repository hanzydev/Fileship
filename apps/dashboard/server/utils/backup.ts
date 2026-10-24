import { promises as fsp } from 'node:fs';

import { nanoid } from 'nanoid';
import { join } from 'pathe';
import { create } from 'tar';

type StoredBackup = { id: string; userId: string; storageId: string | null };

type BackupOwner = { id: string; avatar: string | null; embed: unknown; domains: string[] };

export const backupKey = (backup: { id: string; userId: string }) =>
    `backups/${backup.userId}/${backup.id}.tgz`;

export const serializeBackup = (backup: {
    id: string;
    size: bigint;
    automatic: boolean;
    createdAt: Date;
}) => ({
    id: backup.id,
    automatic: backup.automatic,
    createdAt: backup.createdAt,
    size: {
        raw: backup.size.toString(),
        formatted: formatSize(backup.size),
    },
});

export const backupLimitOf = (owner: NonNullable<PartialUser> & { limits: any }) => {
    const backupLimit = owner.limits?.backupLimit ?? -1;
    return isAdmin(owner) || backupLimit < 0 ? Infinity : backupLimit;
};

export const findBackup = async (userId: string, id: string) => {
    const backup = await prisma.backup.findFirst({ where: { id, userId } });

    if (!backup) {
        throw apiError(404, 'errorsSystem.backupNotFound');
    }

    return backup;
};

export const removeBackupData = (backups: StoredBackup[]) =>
    Promise.allSettled(
        backups.map(async (backup) =>
            (await getStorage(backup.storageId)).remove(backupKey(backup)),
        ),
    );

export const createBackup = async (owner: BackupOwner, automatic = false) => {
    const [files, folders, notes, views, links] = await prisma.$transaction([
        prisma.file.findMany({
            where: { authorId: owner.id, deletedAt: null },
            orderBy: { createdAt: 'desc' },
            omit: {
                embedding: true,
                textEmbedding: true,
                ocrText: true,
                caption: true,
            },
        }),
        prisma.folder.findMany({
            where: { authorId: owner.id, deletedAt: null },
            orderBy: { createdAt: 'desc' },
            include: { inbox: true },
        }),
        prisma.note.findMany({
            where: { authorId: owner.id, deletedAt: null },
            orderBy: { createdAt: 'desc' },
        }),
        prisma.view.findMany({
            where: { ownerId: owner.id },
        }),
        prisma.link.findMany({
            where: { authorId: owner.id },
        }),
    ]);

    const parents = new Map(folders.map((folder) => [folder.id, folder.parentId]));
    const depthOf = (id: string | null, level = 0): number =>
        id && parents.has(id) && level < MAX_FOLDER_DEPTH
            ? depthOf(parents.get(id)!, level + 1)
            : level;

    folders.sort((a, b) => depthOf(a.id) - depthOf(b.id));

    const tempPath = join(dataDirectory, 'temp', nanoid());
    const backupUploadsPath = join(tempPath, 'uploads');
    const backupDatabasePath = join(tempPath, 'database');

    await fsp.mkdir(tempPath);

    try {
        await fsp.mkdir(backupUploadsPath);
        await fsp.mkdir(backupDatabasePath);

        const jsonWriteTasks = [
            { key: 'file', data: files },
            {
                key: 'folder',
                data: folders.map((folder) => ({
                    ...folder,
                    isInbox: !!folder.inbox,
                    inbox: undefined,
                })),
            },
            { key: 'note', data: notes },
            { key: 'view', data: views },
            { key: 'link', data: links },
            {
                key: 'user',
                data: {
                    avatar: owner.avatar,
                    embed: owner.embed,
                    domains: owner.domains,
                },
            },
        ];

        await Promise.all(
            jsonWriteTasks.map(({ key, data }) =>
                fsp.writeFile(
                    join(backupDatabasePath, `${key}.json`),
                    JSON.stringify(data, (_, v) => (typeof v === 'bigint' ? v.toString() : v)),
                ),
            ),
        );

        await Promise.allSettled(
            files.map(async (file) =>
                (await getStorage(file.storageId)).download(
                    uploadKey(file.fileName),
                    join(backupUploadsPath, file.fileName),
                ),
            ),
        );

        const backupCompressedPath = join(tempPath, 'backup.tgz');

        await create({ file: backupCompressedPath, cwd: tempPath, gzip: { level: 3 } }, [
            'uploads',
            'database',
        ]);

        const { size } = await fsp.stat(backupCompressedPath);
        const storageId = await getActiveStorageId();
        const backup = { id: nanoid(), userId: owner.id, size, storageId, automatic };

        await (await getStorage(storageId)).put(backupKey(backup), backupCompressedPath);

        const created = await prisma.backup.upsert({
            where: { id: backup.id },
            create: backup,
            update: { size, storageId, automatic, createdAt: new Date() },
        });

        sendToUser(owner.id, 'backup:create', serializeBackup(created));

        return created;
    } finally {
        await fsp.rm(tempPath, { recursive: true, force: true });
    }
};
