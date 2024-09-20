export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;

    if (!currentUser) {
        throw createError({
            statusCode: 401,
            statusMessage: 'Unauthorized',
            message: 'You do not have permission to perform this action',
        });
    }

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
            },
        },
    });

    const reqUrl = getRequestURL(event);

    const protocol = process.env.NUXT_PUBLIC_RETURN_HTTPS
        ? process.env.NUXT_PUBLIC_RETURN_HTTPS === 'true'
            ? 'https'
            : 'http'
        : reqUrl.protocol.slice(0, -1);

    const domain = currentUser.domains.length
        ? currentUser.domains[
              Math.floor(Math.random() * currentUser.domains.length)
          ]
        : reqUrl.host;

    return folders.map((folder) => ({
        ...folder,
        files: folder.files
            .sort((a, b) => b.createdAt.getTime() - a.createdAt.getTime())
            .map((file) => file.id),
        publicUrl: folder.public
            ? `${protocol}://${domain}/folder/${folder.id}`
            : undefined,
    }));
});
