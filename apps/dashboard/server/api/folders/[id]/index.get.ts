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
                },
            },
        },
    });

    if (!findFolderById) {
        throw createError({
            status: 404,
            message: 'Folder not found',
        });
    }

    const isOwner = findFolderById.authorId === currentUser?.id;

    if (!isOwner && !findFolderById.public) throw forbiddenError;

    const visibleFiles = {
        folderId: findFolderById.id,
        deletedAt: null,
        ...(isOwner ? {} : { piiDetected: false }),
    };

    const [fileCount, lockedCount] = await Promise.all([
        prisma.file.count({ where: visibleFiles }),
        isOwner ? 0 : prisma.file.count({ where: { ...visibleFiles, password: { not: null } } }),
    ]);

    return {
        ...findFolderById,
        inbox: undefined,
        author: undefined,
        authorId: isOwner ? findFolderById.authorId : undefined,
        fileCount,
        lockedCount,
        isInbox: !!findFolderById.inbox,
        embed: defu(findFolderById.author.embed, defaultEmbed) as IEmbed,
        publicUrl: findFolderById.public
            ? buildPublicUrl(event, findFolderById.author.domains, `/folder/${findFolderById.id}`)
            : undefined,
    };
});
