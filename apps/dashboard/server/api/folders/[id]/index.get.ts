export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;
    const folderId = getRouterParam(event, 'id');

    const findFolderById = await prisma.folder.findUnique({
        where: {
            id: folderId,
            deletedAt: null,
        },
        include: {
            inbox: { select: { id: true } },
            author: {
                select: {
                    username: true,
                    avatar: true,
                    domains: true,
                    embed: true,
                    language: true,
                    aiSettings: true,
                },
            },
            _count: { select: { members: true } },
        },
    });

    if (!findFolderById) {
        throw apiError(404, 'errorsContent.folderNotFound');
    }

    const access = await folderAccessOf(findFolderById, currentUser?.id);
    const isOwner = !!access;

    if (!isOwner && !findFolderById.public) throw forbiddenError();

    const visibleFiles = {
        folderId: findFolderById.id,
        deletedAt: null,
        ...(isOwner ? {} : await visitorFileWhere(findFolderById.author)),
    };

    const [fileCount, lockedCount, folderCount, ancestors] = await Promise.all([
        prisma.file.count({ where: visibleFiles }),
        isOwner ? 0 : prisma.file.count({ where: { ...visibleFiles, password: { not: null } } }),
        prisma.folder.count({
            where: {
                parentId: findFolderById.id,
                deletedAt: null,
                ...(!isOwner && { public: true }),
            },
        }),
        folderAncestors(findFolderById.id),
    ]);

    const memberships =
        access && access !== 'owner'
            ? await prisma.folderMember.findMany({
                  where: {
                      userId: currentUser!.id,
                      folderId: { in: ancestors.map((folder) => folder.id) },
                  },
                  select: { folderId: true },
              })
            : [];
    const firstShared = ancestors.findIndex((folder) =>
        memberships.some((membership) => membership.folderId === folder.id),
    );

    const visibleAncestors =
        access === 'owner'
            ? ancestors
            : access
              ? ancestors.slice(firstShared === -1 ? ancestors.length : firstShared)
              : ancestors.slice(ancestors.findLastIndex((folder) => !folder.public) + 1);

    const publicUrl = findFolderById.public
        ? buildPublicUrl(event, findFolderById.author.domains, `/folder/${findFolderById.id}`)
        : undefined;

    return {
        ...findFolderById,
        inbox: undefined,
        author: undefined,
        _count: undefined,
        access: access ?? undefined,
        owner: isOwner
            ? {
                  id: findFolderById.authorId,
                  username: findFolderById.author.username,
                  avatar: findFolderById.author.avatar,
              }
            : undefined,
        memberCount: access === 'owner' ? findFolderById._count.members : undefined,
        authorId: isOwner ? findFolderById.authorId : undefined,
        parentId: isOwner ? findFolderById.parentId : undefined,
        fileCount,
        lockedCount,
        folderCount,
        ancestors: visibleAncestors.map((folder) => ({ id: folder.id, name: folder.name })),
        isInbox: !!findFolderById.inbox,
        embed: embedOf(findFolderById.author.embed),
        publicUrl,
        discordEmbed: publicUrl
            ? discordEmbedFor(
                  'folder',
                  findFolderById.author,
                  { name: findFolderById.name, fileCount, folderCount, url: publicUrl },
                  findFolderById.createdAt,
              )
            : null,
    };
});
