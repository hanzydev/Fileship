export default defineEventHandler((event) => {
    userOnly(event);

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
