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
        userId?: string;
    }>(`folderArchiveStatus:${folderId}:${jobId}`);

    if (!archiveStatus) {
        throw apiError(404, 'errorsContent.jobNotFound');
    }

    if (archiveStatus.isPrivate && archiveStatus.userId !== currentUser?.id) throw forbiddenError();

    if (!archiveStatus.isDone) {
        throw apiError(400, 'errorsContent.jobNotDone');
    }

    const tempPath = join(dataDirectory, 'temp', jobId!);
    const archivePath = join(tempPath, 'archive.tgz');

    if (!existsSync(archivePath)) {
        await storage.removeItem(`folderArchiveStatus:${folderId}:${jobId}`);
        throw apiError(404, 'errorsContent.archiveNotFound');
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
