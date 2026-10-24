import { defu } from 'defu';

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
                    domains: true,
                    embed: true,
                    aiSettings: true,
                },
            },
        },
    });

    if (!findFolderById) {
        throw apiError(404, 'errorsContent.folderNotFound');
    }

    const isOwner = findFolderById.authorId === currentUser?.id;

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

    const visibleAncestors = isOwner
        ? ancestors
        : ancestors.slice(ancestors.findLastIndex((folder) => !folder.public) + 1);

    return {
        ...findFolderById,
        inbox: undefined,
        author: undefined,
        authorId: isOwner ? findFolderById.authorId : undefined,
        parentId: isOwner ? findFolderById.parentId : undefined,
        fileCount,
        lockedCount,
        folderCount,
        ancestors: visibleAncestors.map((folder) => ({ id: folder.id, name: folder.name })),
        isInbox: !!findFolderById.inbox,
        embed: defu(findFolderById.author.embed, defaultEmbed) as IEmbed,
        publicUrl: findFolderById.public
            ? buildPublicUrl(event, findFolderById.author.domains, `/folder/${findFolderById.id}`)
            : undefined,
    };
});
