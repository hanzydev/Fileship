import { promises as fsp } from 'node:fs';

import { nanoid } from 'nanoid';
import { join } from 'pathe';
import { create } from 'tar';

export default defineEventHandler(async (event) => {
    userOnly(event);
    const currentUser = event.context.user!;

    if (
        !isAdmin(currentUser) &&
        currentUser.limits.backupLimit > -1 &&
        (await prisma.backup.count({ where: { userId: currentUser.id } })) >=
            currentUser.limits.backupLimit
    ) {
        throw apiError(400, 'errorsUsers.backupLimitReached');
    }

    const [files, folders, notes, views, links] = await prisma.$transaction([
        prisma.file.findMany({
            where: { authorId: currentUser.id, deletedAt: null },
            orderBy: { createdAt: 'desc' },
            omit: {
                embedding: true,
                textEmbedding: true,
                ocrText: true,
                caption: true,
            },
        }),
        prisma.folder.findMany({
            where: { authorId: currentUser.id, deletedAt: null },
            orderBy: { createdAt: 'desc' },
            include: { inbox: true },
        }),
        prisma.note.findMany({
            where: { authorId: currentUser.id, deletedAt: null },
            orderBy: { createdAt: 'desc' },
        }),
        prisma.view.findMany({
            where: { ownerId: currentUser.id },
        }),
        prisma.link.findMany({
            where: { authorId: currentUser.id },
        }),
    ]);

    const tempPath = join(dataDirectory, 'temp', nanoid());
    const backupUploadsPath = join(tempPath, 'uploads');
    const backupDatabasePath = join(tempPath, 'database');

    await fsp.mkdir(tempPath);
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
                avatar: currentUser.avatar,
                embed: currentUser.embed,
                domains: currentUser.domains,
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

    event.waitUntil(
        create({ file: backupCompressedPath, cwd: tempPath, gzip: { level: 3 } }, [
            'uploads',
            'database',
        ])
            .then(async () => {
                const { size } = await fsp.stat(backupCompressedPath);
                const storageId = await getActiveStorageId();
                const backup = { id: nanoid(), userId: currentUser.id, size, storageId };

                await (await getStorage(storageId)).put(backupKey(backup), backupCompressedPath);

                const created = await prisma.backup.upsert({
                    where: { id: backup.id },
                    create: backup,
                    update: { size, storageId, createdAt: new Date() },
                });

                await createLog(event, {
                    action: 'Create Backup',
                    message: `Created backup ${created.id}`,
                });

                sendToUser(currentUser.id, 'backup:create', serializeBackup(created));
            })
            .finally(() => fsp.rm(tempPath, { recursive: true, force: true })),
    );
});
