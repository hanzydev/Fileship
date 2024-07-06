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

    const noteId = getRouterParam(event, 'id');

    const findNoteById = await prisma.note.findUnique({
        where: {
            id: noteId,
        },
    });

    if (!findNoteById) {
        throw createError({
            statusCode: 404,
            statusMessage: 'Not Found',
            message: 'Note not found',
        });
    }

    if (findNoteById.authorId !== currentUser.id) {
        throw createError({
            statusCode: 403,
            statusMessage: 'Forbidden',
            message: 'You do not have permission to perform this action',
        });
    }

    await prisma.note.delete({
        where: {
            id: noteId,
        },
    });

    const log = await prisma.log.create({
        data: {
            action: 'Delete Note',
            userId: currentUser.id,
            message: `Deleted ${findNoteById.title}`,
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

    sendToUser(currentUser.id, 'delete:note', noteId);
    await sendByFilter(
        (socket) => isAdmin(socket.handshake.auth.user)!,
        'create:log',
        log,
    );
});
