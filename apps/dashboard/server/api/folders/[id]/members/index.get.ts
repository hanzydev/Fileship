export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const { folder } = await findFolderWithAccess(
        getRouterParam(event, 'id'),
        currentUser.id,
        'viewer',
    );

    const members = await prisma.folderMember.findMany({
        where: { folderId: folder.id },
        orderBy: { createdAt: 'asc' },
        include: { user: { select: { username: true, avatar: true } } },
    });

    return members.map(serializeFolderMember);
});
