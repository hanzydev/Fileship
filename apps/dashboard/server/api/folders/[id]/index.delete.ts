import { remove } from '@orama/orama';

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const folderId = getRouterParam(event, 'id');

    const findFolderById = await prisma.folder.findUnique({
        where: {
            id: folderId,
            authorId: currentUser.id,
        },
        include: {
            files: true,
        },
    });

    if (!findFolderById) {
        throw createError({
            statusCode: 404,
            message: 'Folder not found',
        });
    }

    await prisma.folder.delete({
        where: {
            id: folderId,
        },
    });

    await remove(folderSearchDb, folderId!);

    await createLog(event, {
        action: 'Delete Folder',
        message: `Deleted folder ${findFolderById.name}`,
    });

    findFolderById.files.forEach((f) => {
        sendToUser(currentUser.id, 'folder:file:remove', {
            folderId,
            fileId: f.id,
        });
    });

    sendToUser(currentUser.id, 'delete:folder', folderId);
});
