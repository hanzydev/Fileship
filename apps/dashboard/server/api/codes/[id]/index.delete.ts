import { remove } from '@orama/orama';

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const codeId = getRouterParam(event, 'id');

    const findCodeById = await prisma.code.findUnique({
        where: {
            id: codeId,
            authorId: currentUser.id,
        },
    });

    if (!findCodeById) {
        throw createError({
            statusCode: 404,
            message: 'Code not found',
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

    await remove(codeSearchDb, codeId!);

    await createLog(event, {
        action: 'Delete Code',
        message: `Deleted code ${findCodeById.title}`,
    });

    sendToUser(currentUser.id, 'delete:code', codeId);
});
