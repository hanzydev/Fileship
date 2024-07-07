import { existsSync, promises as fsp } from 'node:fs';

import { filesize } from 'filesize';
import { basename, extname, join } from 'pathe';
import { z } from 'zod';

import { sendByFilter, sendToUser } from '~~/server/plugins/socketIO';
import { isAdmin } from '~~/utils/user';

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
    const currentUser = event.context.user;
    if (!currentUser) {
        throw createError({
            statusCode: 401,
            statusMessage: 'Unauthorized',
            message: 'You do not have permission to perform this action',
        });
    }

    const formData = await readFormData(event);

    const backup = formData.get('backup');
    if (!backup) {
        throw createError({
            statusCode: 400,
            statusMessage: 'Bad Request',
            message: 'Missing backup',
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
            data: body.error.format(),
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

    const buffer = Buffer.from(await backup.arrayBuffer());

    if (body.data.currentChunk === body.data.totalChunks) {
        if (existsSync(tempPath)) {
            await fsp.appendFile(tempPath, buffer);
        } else {
            await fsp.writeFile(tempPath, buffer);
        }

        const backupPath = join(
            dataDirectory,
            'backups',
            currentUser.id,
            backupName,
        );

        await fsp.rename(tempPath, backupPath);

        const backupStat = await fsp.stat(backupPath);

        const backupObject = {
            id: basename(backupName, extname(backupName)),
            createdAt: backupStat.birthtime,
            size: {
                raw: backupStat.size,
                formatted: filesize(backupStat.size),
            },
        };

        const log = await prisma.log.create({
            data: {
                action: 'Upload Backup',
                userId: currentUser.id,
                message: `Uploaded backup ${backupName}`,
                ip: getRequestIP(event, { xForwardedFor: true })!,
            },
            include: {
                user: {
                    select: {
                        id: true,
                        username: true,
                    },
                },
            },
        });

        sendToUser(currentUser.id, 'create:backup', backupObject);
        await sendByFilter(
            (socket) => isAdmin(socket.handshake.auth.user)!,
            'create:log',
            log,
        );

        return backupObject;
    } else if (body.data.currentChunk === 1) {
        await fsp.writeFile(tempPath, buffer);
    } else {
        await fsp.appendFile(tempPath, buffer);
    }
});
