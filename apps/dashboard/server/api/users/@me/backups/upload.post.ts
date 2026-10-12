import { promises as fsp } from 'node:fs';

import { nanoid } from 'nanoid';
import { basename } from 'pathe';
import { z } from 'zod';

const validationSchema = z.object({
    uploadId: z.string().regex(/^[\w-]{1,64}$/, 'Invalid upload id'),
    totalChunks: z.number().min(1, 'Total chunks must be at least 1'),
    currentChunk: z.number().min(1, 'Current chunk must be at least 1'),
    chunkOffset: z.number().int().min(0, 'Chunk offset must be at least 0'),
});

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const formData = await readFormData(event);

    const backup = formData.get('backup');
    if (!backup) {
        throw createError({
            status: 400,
            message: 'Missing backup file',
        });
    }

    if (typeof backup === 'string') {
        throw createError({
            status: 400,
            message: 'Invalid backup file',
        });
    }

    const body = validationSchema.safeParse({
        uploadId: formData.get('uploadId'),
        currentChunk: +formData.get('currentChunk')!,
        totalChunks: +formData.get('totalChunks')!,
        chunkOffset: +formData.get('chunkOffset')!,
    });

    if (!body.success) {
        throw createError({
            status: 400,
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    const upload = await receiveChunk(
        currentUser.id,
        {
            uploadId: body.data.uploadId,
            part: body.data.currentChunk,
            offset: body.data.chunkOffset,
            isLast: body.data.currentChunk === body.data.totalChunks,
            data: new Uint8Array(await backup.arrayBuffer()),
        },
        async () => {
            if (!backup.name.endsWith('.tgz')) {
                throw createError({
                    status: 400,
                    message: 'Backups must be .tgz files',
                });
            }

            if (
                !isAdmin(currentUser) &&
                currentUser.limits.backupLimit > -1 &&
                (await prisma.backup.count({ where: { userId: currentUser.id } })) >=
                    currentUser.limits.backupLimit
            ) {
                throw createError({
                    status: 400,
                    message: 'Backup limit reached',
                });
            }

            return { key: backupKey({ id: nanoid(), userId: currentUser.id }), isDirect: true };
        },
    );

    if (!upload) return;

    const { path, session } = upload;

    await (
        await getStorage(session.storageId)
    )
        .completeUpload(session.key, session.state)
        .catch(async (error) => {
            await abortUploadSession(path);
            throw error;
        });

    await fsp.rm(`${path}.json`, { force: true });

    const created = await prisma.backup.create({
        data: {
            id: basename(session.key, '.tgz'),
            userId: currentUser.id,
            size: session.received,
            storageId: session.storageId,
        },
    });

    await createLog(event, {
        action: 'Upload Backup',
        message: `Uploaded backup ${created.id}`,
    });

    sendToUser(currentUser.id, 'backup:create', serializeBackup(created));

    return serializeBackup(created);
});
