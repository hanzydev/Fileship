import { mkdir, rm } from 'node:fs/promises';

import { nanoid } from 'nanoid';
import { dirname, join } from 'pathe';
import { create } from 'tar';

const INVALID_SEGMENT = /[\\/:*?"<>|\p{Cc}]/gu;

const archiveSegment = (name: string, taken: Set<string>) => {
    const base =
        name
            .replace(INVALID_SEGMENT, '')
            .trim()
            .replace(/^\.+|[. ]+$/g, '') || 'Folder';
    let candidate = base;

    for (let index = 2; taken.has(candidate.toLowerCase()); index++) {
        candidate = `${base} (${index})`;
    }

    taken.add(candidate.toLowerCase());
    return candidate;
};

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
            author: { select: { aiSettings: true } },
        },
    });

    if (!findFolderById) {
        throw apiError(404, 'errorsContent.folderNotFound');
    }

    const isPrivate = !!(await folderAccessOf(findFolderById, currentUser?.id));

    if (!isPrivate) {
        if (!findFolderById.public) throw forbiddenError();

        await rateLimit(`folderArchive:${getClientIp(event)}`, 5, 10 * 60_000);
    }

    const paths = new Map<string, string>([[findFolderById.id, '']]);
    const names = new Map<string, Set<string>>();

    for (const folder of await folderSubtree([findFolderById.id])) {
        const parentPath = folder.parentId === null ? undefined : paths.get(folder.parentId);
        if (!folder.level || parentPath === undefined || (!isPrivate && !folder.public)) continue;

        const taken = names.get(folder.parentId!) ?? new Set();
        names.set(folder.parentId!, taken);
        paths.set(folder.id, join(parentPath, archiveSegment(folder.name, taken)));
    }

    const files = (
        await prisma.file.findMany({
            where: {
                folderId: { in: [...paths.keys()] },
                deletedAt: null,
                ...(!isPrivate && {
                    password: null,
                    ...(await visitorFileWhere(findFolderById.author)),
                }),
            },
            orderBy: { createdAt: 'desc' },
            select: { id: true, fileName: true, folderId: true, storageId: true },
        })
    ).map((file) => ({ ...file, path: join(paths.get(file.folderId!)!, file.fileName) }));

    const keys = await storage.getKeys(`folderArchiveStatus:${folderId}:`);
    const items = await storage.getItems<{ isDone: boolean }>(keys);

    const isAnyJobRunning = items.some((item) => item.value && !item.value.isDone);
    if (isAnyJobRunning) {
        throw apiError(400, 'errorsContent.archiveRunning');
    }

    if (!files.length) {
        throw apiError(400, 'errorsContent.nothingToArchive');
    }

    const jobId = nanoid();

    await storage.setItem(`folderArchiveStatus:${folderId}:${jobId}`, {
        isPrivate,
        isDone: false,
        userId: currentUser?.id,
    });

    const tempPath = join(dataDirectory, 'temp', jobId);
    const archiveUploadsPath = join(tempPath, 'uploads');

    await mkdir(tempPath);
    await mkdir(archiveUploadsPath);

    const downloads = await Promise.allSettled(
        files.map(async (file) => {
            await mkdir(join(archiveUploadsPath, dirname(file.path)), { recursive: true });
            await (
                await getStorage(file.storageId)
            ).download(uploadKey(file.fileName), join(archiveUploadsPath, file.path));

            return file.path;
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
                    userId: currentUser?.id,
                });
            })
            .catch(() => storage.removeItem(`folderArchiveStatus:${folderId}:${jobId}`)),
    );

    return {
        jobId,
    };
});
