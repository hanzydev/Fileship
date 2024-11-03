import { existsSync, promises as fsp } from 'node:fs';

import { filesize } from 'filesize';
import { basename, extname, join } from 'pathe';
import { z } from 'zod';

const validationSchema = z.object(
    {
        totalChunks: z
            .number({
                invalid_type_error: 'Invalid total chunks',
                required_error: 'Missing total chunks',
            })
            .min(1, 'Total chunks must be at least 1')
            .optional(),
        currentChunk: z
            .number({
                invalid_type_error: 'Invalid current chunk',
                required_error: 'Missing current chunk',
            })
            .min(1, 'Current chunk must be at least 1')
            .optional(),
    },
    { invalid_type_error: 'Invalid body', required_error: 'Missing body' },
);

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const formData = await readFormData(event);

    const backup = formData.get('backup');
    if (!backup) {
        throw createError({
            statusCode: 400,
            statusMessage: 'Bad Request',
            message: 'Missing backup file',
        });
    }

    if (typeof backup === 'string') {
        throw createError({
            statusCode: 400,
            statusMessage: 'Bad Request',
            message: 'Invalid backup file',
        });
    }

    const body = validationSchema.safeParse({
        currentChunk: +formData.get('currentChunk')! || 1,
        totalChunks: +formData.get('totalChunks')! || 1,
    });

    if (!body.success) {
        throw createError({
            statusCode: 400,
            statusMessage: 'Bad Request',
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    const userBackupsPath = join(dataDirectory, 'backups', currentUser.id);
    if (!existsSync(userBackupsPath)) await fsp.mkdir(userBackupsPath);

    const userBackups = await fsp.readdir(userBackupsPath);
    if (userBackups.includes(backup.name)) {
        throw createError({
            statusCode: 409,
            statusMessage: 'Conflict',
            message: 'A backup with that name already exists',
        });
    }

    const backupName = backup.name.replace(/[^a-zA-Z0-9-_.]/g, '');

    const tempPath = join(dataDirectory, 'temp', currentUser.id, backupName);
    if (!existsSync(join(dataDirectory, 'temp', currentUser.id)))
        await fsp.mkdir(join(dataDirectory, 'temp', currentUser.id));

    const buffer = new Uint8Array(await backup.arrayBuffer());

    if (body.data.currentChunk === body.data.totalChunks) {
        if (existsSync(tempPath)) {
            await fsp.appendFile(tempPath, buffer);
        } else {
            await fsp.writeFile(tempPath, buffer);
        }

        const backupPath = join(dataDirectory, 'backups', currentUser.id, backupName);

        await fsp.rename(tempPath, backupPath);

        const backupStat = await fsp.stat(backupPath);
        const id = basename(backupName, extname(backupName));

        const backupObject = {
            id,
            createdAt: backupStat.birthtime,
            size: {
                raw: backupStat.size,
                formatted: filesize(backupStat.size),
            },
        };

        await createLog(event, {
            action: 'Upload Backup',
            message: `Uploaded backup ${id}`,
        });

        sendToUser(currentUser.id, 'create:backup', backupObject);

        return backupObject;
    } else if (body.data.currentChunk === 1) {
        await fsp.writeFile(tempPath, buffer);
    } else {
        await fsp.appendFile(tempPath, buffer);
    }
});
