import { existsSync, promises as fsp } from 'node:fs';

import { filesize } from 'filesize';
import { nanoid } from 'nanoid';
import { join } from 'pathe';
import { create } from 'tar';

import { sendToUser } from '~~/server/plugins/socketIO';
import { isAdmin } from '~~/utils/user';

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;

    if (!currentUser) {
        throw createError({
            statusCode: 401,
            statusMessage: 'Unauthorized',
            message: 'You do not have permission to perform this action',
        });
    }

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

    const backupId = nanoid();
    const backupPath = join(userBackupsPath, `${backupId}.tgz`);

    create(
        {
            file: backupPath,
            cwd: tempPath,
            gzip: { level: 9 },
        },
        ['uploads', 'database'],
    ).then(async () => {
        await fsp.rm(tempPath, { recursive: true });

        const backupStat = await fsp.stat(backupPath);

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
