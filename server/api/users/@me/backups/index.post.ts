import { existsSync, promises as fsp } from 'node:fs';

import { filesize } from 'filesize';
import { nanoid } from 'nanoid';
import { join } from 'pathe';
import { create } from 'tar';

import { isAdmin } from '~~/utils/permissions';

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

    const userData = await prisma.$transaction([
        prisma.file.findMany({
            where: {
                authorId: currentUser.id,
            },
        }),
        prisma.folder.findMany({
            where: {
                authorId: currentUser.id,
            },
        }),
        prisma.note.findMany({
            where: {
                authorId: currentUser.id,
            },
        }),
        prisma.code.findMany({
            where: {
                authorId: currentUser.id,
            },
        }),
        prisma.url.findMany({
            where: {
                authorId: currentUser.id,
            },
        }),
        prisma.view.findMany({
            where: {
                OR: [
                    {
                        file: {
                            authorId: currentUser.id,
                        },
                    },
                    {
                        code: {
                            authorId: currentUser.id,
                        },
                    },
                    {
                        url: {
                            authorId: currentUser.id,
                        },
                    },
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

    for (const { key, data } of [
        {
            key: 'file',
            data: userData[0],
        },
        {
            key: 'folder',
            data: userData[1],
        },
        {
            key: 'note',
            data: userData[2],
        },
        {
            key: 'code',
            data: userData[3],
        },
        {
            key: 'url',
            data: userData[4],
        },
        {
            key: 'view',
            data: userData[5],
        },
        {
            key: 'user',
            data: {
                avatar: currentUser.avatar,
                embed: currentUser.embed,
                domains: currentUser.domains,
            },
        },
    ]) {
        await fsp.writeFile(
            join(backupDatabasePath, `${key}.json`),
            JSON.stringify(data, (_, value) =>
                typeof value === 'bigint' ? value.toString() : value,
            ),
        );
    }

    const userUploads = (await fsp.readdir(uploadsPath)).filter((file) =>
        userData[0].some((upload) => upload.fileName === file),
    );

    const cpPromises = userUploads.map((file) =>
        fsp.cp(join(uploadsPath, file), join(backupUploadsPath, file)),
    );

    await Promise.all(cpPromises);

    const backupCompressedPath = join(tempPath, 'backup.tgz');

    create(
        {
            file: backupCompressedPath,
            cwd: tempPath,
            gzip: { level: 5 },
        },
        ['uploads', 'database'],
    ).then(async () => {
        const backupStat = await fsp.stat(backupCompressedPath);

        const backupId = nanoid();
        await fsp.rename(
            backupCompressedPath,
            join(userBackupsPath, `${backupId}.tgz`),
        );
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
