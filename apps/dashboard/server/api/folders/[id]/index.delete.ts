import z from 'zod';

const validationSchema = z.object({
    deleteFilesToo: z.boolean().optional(),
    permanent: z.boolean().optional(),
});

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const folderId = getRouterParam(event, 'id');
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw invalidBodyError(body.error);
    }

    const findFolderById = await prisma.folder.findUnique({
        where: {
            id: folderId,
            deletedAt: null,
        },
    });

    const isOwner = findFolderById?.authorId === currentUser.id;
    const canDelete =
        isOwner ||
        (!!findFolderById?.parentId &&
            hasFolderAccess(
                await folderAccessOf(
                    { id: findFolderById.parentId, authorId: findFolderById.authorId },
                    currentUser.id,
                ),
                'editor',
            ));

    if (!findFolderById || !canDelete) {
        throw findFolderById && (await folderAccessOf(findFolderById, currentUser.id))
            ? forbiddenError()
            : apiError(404, 'errorsContent.folderNotFound');
    }

    const folderIds = (await folderSubtree([findFolderById.id])).map((folder) => folder.id);
    const audience = await folderAudience(findFolderById.authorId, [findFolderById.id], folderIds);

    if (await prisma.inbox.findFirst({ where: { folderId: { in: folderIds } } })) {
        throw apiError(400, 'errorsContent.inboxFolderDelete');
    }

    const files = await prisma.file.findMany({
        where: { folderId: { in: folderIds }, deletedAt: null },
        include: { folder: { select: { public: true } } },
    });

    const isTrashed = !(isOwner && body.data.permanent) && (await isTrashEnabled());
    const keptFiles = { folderId: findFolderById.parentId };

    if (isTrashed) {
        const deletedAt = new Date();

        await prisma.$transaction([
            prisma.file.updateMany({
                where: { id: { in: files.map((file) => file.id) } },
                data: body.data.deleteFilesToo ? { deletedAt } : keptFiles,
            }),
            prisma.folder.updateMany({ where: { id: { in: folderIds } }, data: { deletedAt } }),
        ]);
    } else {
        if (body.data.deleteFilesToo) await deleteFilesForever(files);
        else
            await prisma.file.updateMany({
                where: { id: { in: files.map((file) => file.id) } },
                data: keptFiles,
            });

        await deleteFoldersForever(folderIds);
    }

    if (body.data.deleteFilesToo) {
        emitFilesDeleted(event, findFolderById.authorId, files, isTrashed ? 'trashed' : 'deleted');

        const deletionPromises = files.map(async (file) => {
            await createLog(event, {
                action: 'Delete File',
                message: isTrashed
                    ? `Moved file ${file.fileName} to trash`
                    : `Deleted file ${file.fileName}`,
            });

            for (const userId of audience) sendToUser(userId, 'file:delete', file.id);
        });

        await Promise.allSettled(deletionPromises);
    }

    await createLog(event, {
        action: 'Delete Folder',
        message: isTrashed
            ? `Moved folder ${findFolderById.name} to trash`
            : `Deleted folder ${findFolderById.name}`,
    });

    for (const userId of audience) {
        for (const id of folderIds) sendToUser(userId, 'folder:delete', id);
    }

    sendToUser(findFolderById.authorId, 'trash:update', null);
});
