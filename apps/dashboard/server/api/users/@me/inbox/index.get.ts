export default defineEventHandler((event) => {
    fileUploaderOnly(event);

    const currentUser = event.context.user!;

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
    })!;
});
