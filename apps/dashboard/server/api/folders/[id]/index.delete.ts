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
            authorId: currentUser.id,
            deletedAt: null,
        },
    });

    if (!findFolderById) {
        throw apiError(404, 'errorsContent.folderNotFound');
    }

    const folderIds = (await folderSubtree([findFolderById.id])).map((folder) => folder.id);

    if (await prisma.inbox.findFirst({ where: { folderId: { in: folderIds } } })) {
        throw apiError(400, 'errorsContent.inboxFolderDelete');
    }

    const files = await prisma.file.findMany({
        where: { folderId: { in: folderIds }, deletedAt: null },
    });

    const isTrashed = !body.data.permanent && (await isTrashEnabled());
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
        const deletionPromises = files.map(async (file) => {
            await createLog(event, {
                action: 'Delete File',
                message: isTrashed
                    ? `Moved file ${file.fileName} to trash`
                    : `Deleted file ${file.fileName}`,
            });

            sendToUser(currentUser.id, 'file:delete', file.id);
        });

        await Promise.allSettled(deletionPromises);
    }

    await createLog(event, {
        action: 'Delete Folder',
        message: isTrashed
            ? `Moved folder ${findFolderById.name} to trash`
            : `Deleted folder ${findFolderById.name}`,
    });

    for (const id of folderIds) sendToUser(currentUser.id, 'folder:delete', id);
    sendToUser(currentUser.id, 'trash:update', null);
});
