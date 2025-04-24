export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;

    const folders = await prisma.folder.findMany({
        where: {
            authorId: currentUser.id,
        },
        orderBy: {
            createdAt: 'desc',
        },
        include: {
            files: {
                select: {
                    id: true,
                    createdAt: true,
                },
                orderBy: {
                    createdAt: 'desc',
                },
            },
        },
    });

    return folders.map((folder) => ({
        ...folder,
        files: folder.files
            .sort((a, b) => b.createdAt.getTime() - a.createdAt.getTime())
            .map((file) => file.id),
        publicUrl: folder.public
            ? buildPublicUrl(event, currentUser.domains, `/folder/${folder.id}`)
            : undefined,
    }));
});
