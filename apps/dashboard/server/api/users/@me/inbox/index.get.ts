export default defineEventHandler(async (event) => {
    fileUploaderOnly(event);

    const currentUser = event.context.user!;

    await ensureInbox(currentUser.id);

    return prisma.inbox.findUnique({
        where: {
            userId: currentUser.id,
        },
        select: {
            id: true,
            enabled: true,
            folderId: true,
            password: true,
            slug: true,
        },
    });
});
