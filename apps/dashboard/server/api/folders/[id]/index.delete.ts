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
        include: {
            files: { where: { deletedAt: null } },
            inbox: true,
        },
    });

    if (!findFolderById) {
        throw apiError(404, 'errorsContent.folderNotFound');
    }

    if (findFolderById.inbox) {
        throw apiError(400, 'errorsContent.inboxFolderDelete');
    }

    const isTrashed = !body.data.permanent && (await isTrashEnabled());

    if (isTrashed) {
        const deletedAt = new Date();

        await prisma.$transaction([
            prisma.file.updateMany({
                where: { id: { in: findFolderById.files.map((file) => file.id) } },
                data: body.data.deleteFilesToo ? { deletedAt } : { folderId: null },
            }),
            prisma.folder.update({ where: { id: folderId }, data: { deletedAt } }),
        ]);
    } else {
        if (body.data.deleteFilesToo) await deleteFilesForever(findFolderById.files);
        await deleteFoldersForever([folderId!]);
    }

    if (body.data.deleteFilesToo) {
        const deletionPromises = findFolderById.files.map(async (file) => {
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

    sendToUser(currentUser.id, 'folder:delete', folderId);
    sendToUser(currentUser.id, 'trash:update', null);
});
