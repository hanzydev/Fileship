import { sendToUser } from '~~/server/plugins/socketIO';

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;
    if (!currentUser) {
        throw createError({
            statusCode: 401,
            statusMessage: 'Unauthorized',
            message: 'You do not have permission to perform this action',
        });
    }

    const codeId = getRouterParam(event, 'id');

    const findCodeById = await prisma.code.findUnique({
        where: {
            id: codeId,
        },
    });

    if (!findCodeById) {
        throw createError({
            statusCode: 404,
            statusMessage: 'Not Found',
            message: 'Code not found',
        });
    }

    if (findCodeById.authorId !== currentUser.id) {
        throw createError({
            statusCode: 403,
            statusMessage: 'Forbidden',
            message: 'You do not have permission to perform this action',
        });
    }

    await prisma.view.deleteMany({
        where: {
            codeId,
        },
    });

    await prisma.code.delete({
        where: {
            id: codeId,
        },
    });

    await createLog(event, {
        action: 'Delete Code',
        message: `Deleted code ${findCodeById.title}`,
    });

    sendToUser(currentUser.id, 'delete:code', codeId);
});
