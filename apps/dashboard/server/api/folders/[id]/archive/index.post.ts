import { cp, mkdir, readdir, rm } from 'node:fs/promises';

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
        },
        include: {
            files: {
                orderBy: {
                    createdAt: 'desc',
                },
                select: {
                    fileName: true,
                    password: true,
                },
            },
        },
    });

    if (!findFolderById) {
        throw createError({
            statusCode: 404,
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
            statusCode: 400,
            message: 'An archive job is already running for this folder',
        });
    }

    const jobId = nanoid();

    const tempPath = join(dataDirectory, 'temp', jobId);
    const archiveUploadsPath = join(tempPath, 'uploads');
    const uploadsPath = join(dataDirectory, 'uploads');

    await mkdir(tempPath);
    await mkdir(archiveUploadsPath);

    const userUploads = (await readdir(uploadsPath)).filter((file) =>
        findFolderById.files.some((upload) => upload.fileName === file),
    );

    await Promise.all(
        userUploads.map((file) => cp(join(uploadsPath, file), join(archiveUploadsPath, file))),
    );

    const archiveCompressedPath = join(tempPath, 'archive.tgz');

    await storage.setItem(`folderArchiveStatus:${folderId}:${jobId}`, {
        isPrivate,
        isDone: false,
        authorId: findFolderById.authorId,
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
