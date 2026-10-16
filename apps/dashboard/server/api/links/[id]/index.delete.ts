export default defineEventHandler(async (event) => {
    linkShortenerOnly(event);

    const currentUser = event.context.user!;
    const findLinkById = await prisma.link.findUnique({
        where: { id: getRouterParam(event, 'id'), authorId: currentUser.id },
    });

    if (!findLinkById) {
        throw apiError(404, 'errorsContent.linkNotFound');
    }

    await prisma.link.delete({ where: { id: findLinkById.id } });

    await createLog(event, {
        action: 'Delete Link',
        message: `Deleted link ${findLinkById.code}`,
    });

    sendToUser(currentUser.id, 'link:delete', findLinkById.id);
});
