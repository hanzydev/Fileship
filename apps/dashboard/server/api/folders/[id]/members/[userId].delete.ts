export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const userId = getRouterParam(event, 'userId')!;
    const { folder, access } = await findFolderWithAccess(
        getRouterParam(event, 'id'),
        currentUser.id,
        'viewer',
    );

    if (access !== 'owner' && userId !== currentUser.id) throw forbiddenError();

    const member = await prisma.folderMember.findUnique({
        where: { folderId_userId: { folderId: folder.id, userId } },
        include: { user: { select: { username: true } } },
    });

    if (!member) throw apiError(404, 'errorsContent.memberNotFound');

    await prisma.folderMember.delete({ where: { id: member.id } });

    await createLog(event, {
        action: 'Unshare Folder',
        message:
            userId === currentUser.id
                ? `Left folder ${folder.name}`
                : `Stopped sharing folder ${folder.name} with ${member.user.username}`,
    });

    for (const id of new Set([folder.authorId, userId])) {
        sendToUser(id, 'folder:share', { folderId: folder.id });
    }
});
