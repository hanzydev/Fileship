import { createReadStream, existsSync, promises as fsp } from 'node:fs';
import { finished } from 'node:stream/promises';

import { join } from 'pathe';

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;
    const folderId = getRouterParam(event, 'id');
    const jobId = getRouterParam(event, 'jobId');
    const storage = useStorage('cache');

    const archiveStatus = await storage.getItem<{
        isPrivate: boolean;
        isDone: boolean;
        authorId: string;
    }>(`folderArchiveStatus:${folderId}:${jobId}`);

    if (!archiveStatus) {
        throw createError({
            statusCode: 404,
            message: 'Job not found',
        });
    }

    if (archiveStatus.isPrivate && archiveStatus.authorId !== currentUser?.id) throw forbiddenError;

    if (!archiveStatus.isDone) {
        throw createError({
            statusCode: 400,
            message: 'Job is not done yet',
        });
    }

    const tempPath = join(dataDirectory, 'temp', jobId!);
    const archivePath = join(tempPath, 'archive.tgz');

    if (!existsSync(archivePath)) {
        await storage.removeItem(`folderArchiveStatus:${folderId}:${jobId}`);
        throw createError({
            statusCode: 404,
            message: 'Archive not found',
        });
    }

    const archiveStat = await fsp.stat(archivePath);

    setResponseHeaders(event, {
        'Content-Type': 'application/gzip',
        'Content-Length': archiveStat.size.toString(),
        'Accept-Ranges': 'bytes',
    });

    setResponseHeader(
        event,
        'Content-Disposition',
        `attachment; filename="folderArchive-${folderId}-${jobId}.tgz"`,
    );

    const stream = createReadStream(archivePath);

    finished(stream).then(async () => {
        try {
            await storage.removeItem(`folderArchiveStatus:${folderId}:${jobId}`);
            await fsp.rm(tempPath, { recursive: true, force: true });
        } catch {
            //
        }
    });

    return sendStream(event, stream);
});
