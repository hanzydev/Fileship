import { promises as fsp } from 'node:fs';

import { nanoid } from 'nanoid';
import { basename } from 'pathe';
import { z } from 'zod';

const validationSchema = z.object({
    uploadId: z.string().regex(/^[\w-]{1,64}$/, 'errorsUsers.invalidUploadId'),
    totalChunks: z
        .number()
        .int()
        .min(1, 'errorsUsers.totalChunksMin')
        .max(10000, 'errorsUsers.tooManyChunks'),
    currentChunk: z.number().int().min(1, 'errorsUsers.currentChunkMin'),
    chunkOffset: z.number().int().min(0, 'errorsUsers.chunkOffsetMin'),
});

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const formData = await readFormData(event);

    const backup = formData.get('backup');
    if (!backup) {
        throw apiError(400, 'errorsUsers.missingBackupFile');
    }

    if (typeof backup === 'string') {
        throw apiError(400, 'errorsUsers.invalidBackupFile');
    }

    const body = validationSchema.safeParse({
        uploadId: formData.get('uploadId'),
        currentChunk: +formData.get('currentChunk')!,
        totalChunks: +formData.get('totalChunks')!,
        chunkOffset: +formData.get('chunkOffset')!,
    });

    if (!body.success) {
        throw invalidBodyError(body.error);
    }

    const upload = await receiveChunk(
        currentUser.id,
        {
            kind: 'backup',
            uploadId: body.data.uploadId,
            part: body.data.currentChunk,
            offset: body.data.chunkOffset,
            total: body.data.totalChunks,
            data: new Uint8Array(await backup.arrayBuffer()),
        },
        async () => {
            if (!backup.name.endsWith('.tgz')) {
                throw apiError(400, 'errorsUsers.backupNotTgz');
            }

            if (
                (await prisma.backup.count({ where: { userId: currentUser.id } })) >=
                backupLimitOf(currentUser)
            ) {
                throw apiError(400, 'errorsUsers.backupLimitReached');
            }

            return { key: backupKey({ id: nanoid(), userId: currentUser.id }), isDirect: true };
        },
        (size) => exceedsUsableSpace(currentUser, size),
    );

    if (!upload) return;

    const { path, session, parts } = upload;

    await (
        await getStorage(session.storageId)
    )
        .completeUpload(session.key, session.state, parts)
        .catch(async (error) => {
            await abortUploadSession(path);
            throw error;
        });

    await fsp.rm(`${path}.json`, { force: true });

    const data = {
        id: basename(session.key, '.tgz'),
        userId: currentUser.id,
        size: session.received,
        storageId: session.storageId,
    };

    const created = await prisma.backup.upsert({
        where: { id: data.id },
        create: data,
        update: { size: data.size, storageId: data.storageId, createdAt: new Date() },
    });

    await createLog(event, {
        action: 'Upload Backup',
        message: `Uploaded backup ${created.id}`,
    });

    sendToUser(currentUser.id, 'backup:create', serializeBackup(created));

    return serializeBackup(created);
});
