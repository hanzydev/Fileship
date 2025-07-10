import { existsSync, promises as fsp } from 'node:fs';

import { filesize } from 'filesize';
import { nanoid } from 'nanoid';
import { join } from 'pathe';
import { create } from 'tar';

export default defineEventHandler(async (event) => {
    userOnly(event);
    const currentUser = event.context.user!;

    const userBackupsPath = join(dataDirectory, 'backups', currentUser.id);
    const uploadsPath = join(dataDirectory, 'uploads');

    if (!existsSync(userBackupsPath)) await fsp.mkdir(userBackupsPath);

    const userBackups = await fsp.readdir(userBackupsPath);
    if (
        !isAdmin(currentUser) &&
        currentUser.limits.backupLimit > -1 &&
        userBackups.length >= currentUser.limits.backupLimit
    ) {
        throw createError({
            statusCode: 400,
            statusMessage: 'Bad Request',
            message: 'Backup limit reached',
        });
    }

    const [files, folders, notes, codes, urls, views] = await prisma.$transaction([
        prisma.file.findMany({
            where: { authorId: currentUser.id },
            orderBy: { createdAt: 'desc' },
            omit: {
                embedding: true,
            },
        }),
        prisma.folder.findMany({
            where: { authorId: currentUser.id },
            orderBy: { createdAt: 'desc' },
        }),
        prisma.note.findMany({
            where: { authorId: currentUser.id },
            orderBy: { createdAt: 'desc' },
        }),
        prisma.code.findMany({
            where: { authorId: currentUser.id },
            orderBy: { createdAt: 'desc' },
        }),
        prisma.url.findMany({
            where: { authorId: currentUser.id },
            orderBy: { createdAt: 'desc' },
        }),
        prisma.view.findMany({
            where: {
                OR: [
                    { file: { authorId: currentUser.id } },
                    { code: { authorId: currentUser.id } },
                    { url: { authorId: currentUser.id } },
                ],
            },
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
        { key: 'folder', data: folders },
        { key: 'note', data: notes },
        { key: 'code', data: codes },
        { key: 'url', data: urls },
        { key: 'view', data: views },
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

    const userUploads = (await fsp.readdir(uploadsPath)).filter((file) =>
        files.some((upload) => upload.fileName === file),
    );

    await Promise.all(
        userUploads.map((file) => fsp.cp(join(uploadsPath, file), join(backupUploadsPath, file))),
    );

    const backupCompressedPath = join(tempPath, 'backup.tgz');

    create({ file: backupCompressedPath, cwd: tempPath, gzip: { level: 5 } }, [
        'uploads',
        'database',
    ]).then(async () => {
        const backupStat = await fsp.stat(backupCompressedPath);
        const backupId = nanoid();
        await fsp.rename(backupCompressedPath, join(userBackupsPath, `${backupId}.tgz`));
        await fsp.rm(tempPath, { recursive: true });

        await createLog(event, {
            action: 'Create Backup',
            message: `Created backup ${backupId}`,
        });

        sendToUser(currentUser.id, 'create:backup', {
            id: backupId,
            createdAt: backupStat.birthtime,
            size: {
                raw: backupStat.size,
                formatted: filesize(backupStat.size),
            },
        });
    });
});
