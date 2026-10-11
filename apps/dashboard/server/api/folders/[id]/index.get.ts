import { defu } from 'defu';

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;
    const folderId = getRouterParam(event, 'id');

    const findFolderById = await prisma.folder.findUnique({
        where: {
            id: folderId,
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
            statusCode: 404,
            message: 'Folder not found',
        });
    }

    const isOwner = findFolderById.authorId === currentUser?.id;

    if (!isOwner && !findFolderById.public) throw forbiddenError;

    const fileCount = await prisma.file.count({
        where: {
            folderId: findFolderById.id,
            ...(isOwner ? {} : { password: null, piiDetected: false }),
        },
    });

    return {
        ...findFolderById,
        inbox: undefined,
        author: undefined,
        authorId: isOwner ? findFolderById.authorId : undefined,
        fileCount,
        isInbox: !!findFolderById.inbox,
        embed: defu(findFolderById.author.embed, defaultEmbed) as IEmbed,
        publicUrl: findFolderById.public
            ? buildPublicUrl(event, findFolderById.author.domains, `/folder/${findFolderById.id}`)
            : undefined,
    };
});
