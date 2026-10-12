import { mkdir, rm } from 'node:fs/promises';

import { nanoid } from 'nanoid';
import { join } from 'pathe';
import { create } from 'tar';

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;
    const folderId = getRouterParam(event, 'id');
    const storage = useStorage('cache');

    const findFolderById = await prisma.folder.findUnique({
        where: {
            id: folderId,
            deletedAt: null,
        },
        include: {
            files: {
                where: {
                    deletedAt: null,
                },
                orderBy: {
                    createdAt: 'desc',
                },
                select: {
                    fileName: true,
                    password: true,
                    storageId: true,
                },
            },
        },
    });

    if (!findFolderById) {
        throw createError({
            status: 404,
            message: 'Folder not found',
        });
    }

    let isPrivate = true;

    if (findFolderById.authorId !== currentUser?.id) {
        if (!findFolderById.public) {
            throw forbiddenError;
        } else {
            isPrivate = false;
            findFolderById.files = findFolderById.files.filter((f) => !f.password);
        }
    }

    const keys = await storage.getKeys(`folderArchiveStatus:${folderId}:`);
    const items = await storage.getItems<{ isDone: boolean }>(keys);

    const isAnyJobRunning = items.some((item) => item.value && !item.value.isDone);
    if (isAnyJobRunning) {
        throw createError({
            status: 400,
            message: 'An archive job is already running for this folder',
        });
    }

    if (!findFolderById.files.length) {
        throw createError({
            status: 400,
            message: 'No files to archive in this folder',
        });
    }

    const jobId = nanoid();

    await storage.setItem(`folderArchiveStatus:${folderId}:${jobId}`, {
        isPrivate,
        isDone: false,
        authorId: findFolderById.authorId,
    });

    const tempPath = join(dataDirectory, 'temp', jobId);
    const archiveUploadsPath = join(tempPath, 'uploads');

    await mkdir(tempPath);
    await mkdir(archiveUploadsPath);

    const downloads = await Promise.allSettled(
        findFolderById.files.map(async (file) => {
            await (
                await getStorage(file.storageId)
            ).download(uploadKey(file.fileName), join(archiveUploadsPath, file.fileName));

            return file.fileName;
        }),
    );

    const userUploads = downloads
        .filter((download) => download.status === 'fulfilled')
        .map((download) => download.value);

    const archiveCompressedPath = join(tempPath, 'archive.tgz');

    await createLog(event, {
        action: 'Archive Folder',
        message: `Archived folder ${findFolderById.name}`,
    });

    event.waitUntil(
        create(
            { file: archiveCompressedPath, cwd: archiveUploadsPath, gzip: { level: 3 } },
            userUploads,
        )
            .then(async () => {
                await rm(archiveUploadsPath, { recursive: true });

                await storage.setItem(`folderArchiveStatus:${folderId}:${jobId}`, {
                    isPrivate,
                    isDone: true,
                    authorId: findFolderById.authorId,
                });

                if (findFolderById.authorId === currentUser?.id) {
                    await sendToSession(
                        currentUser.id,
                        currentUser.currentSessionId,
                        'folder:archiveReady',
                        folderId,
                    );
                }
            })
            .catch(() => storage.removeItem(`folderArchiveStatus:${folderId}:${jobId}`)),
    );

    return {
        jobId,
    };
});
