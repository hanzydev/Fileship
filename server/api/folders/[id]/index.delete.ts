import { sendByFilter, sendToUser } from '~~/server/plugins/socketIO';
import { isAdmin } from '~~/utils/user';

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;
    if (!currentUser) {
        throw createError({
            statusCode: 401,
            statusMessage: 'Unauthorized',
            message: 'You do not have permission to perform this action',
        });
    }

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

    const log = await prisma.log.create({
        data: {
            action: 'Delete Folder',
            userId: currentUser.id,
            message: `Deleted ${findFolderById.name}`,
            ip: getRequestIP(event, { xForwardedFor: true })!,
        },
        include: {
            user: {
                select: {
                    id: true,
                    username: true,
                },
            },
        },
    });

    sendToUser(currentUser.id, 'delete:folder', folderId);
    await sendByFilter(
        (socket) => isAdmin(socket.handshake.auth.user)!,
        'create:log',
        log,
    );

    findFolderById.files.forEach((f) => {
        sendToUser(currentUser.id, 'folder:file:remove', {
            folderId,
            fileId: f.id,
        });
    });
});
