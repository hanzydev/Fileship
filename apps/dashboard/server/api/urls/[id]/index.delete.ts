import { remove } from '@orama/orama';

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const urlId = getRouterParam(event, 'id');

    const findUrlById = await prisma.url.findUnique({
        where: {
            id: urlId,
            authorId: currentUser.id,
        },
    });

    if (!findUrlById) {
        throw createError({
            statusCode: 404,
            message: 'URL not found',
        });
    }

    await prisma.view.deleteMany({
        where: {
            urlId,
        },
    });

    await prisma.url.delete({
        where: {
            id: urlId,
        },
    });

    await remove(urlSearchDb, urlId!);

    await createLog(event, {
        action: 'Delete URL',
        message: `Deleted URL ${findUrlById.vanity}`,
    });

    sendToUser(currentUser.id, 'delete:url', urlId);
});
