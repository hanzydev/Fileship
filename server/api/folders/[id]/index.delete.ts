import { sendToUser } from '~~/server/plugins/socketIO';

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const folderId = getRouterParam(event, 'id');

    const findFolderById = await prisma.folder.findUnique({
        where: {
            id: folderId,
        },
        include: {
            files: true,
        },
    });

    if (!findFolderById) {
        throw createError({
            statusCode: 404,
            statusMessage: 'Not Found',
            message: 'Folder not found',
        });
    }

    if (findFolderById.authorId !== currentUser.id) {
        throw createError({
            statusCode: 403,
            statusMessage: 'Forbidden',
            message: 'You do not have permission to perform this action',
        });
    }

    await prisma.folder.delete({
        where: {
            id: folderId,
        },
    });

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
