import { filesize } from 'filesize';

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;

    if (!currentUser) {
        throw createError({
            statusCode: 401,
            statusMessage: 'Unauthorized',
            message: 'You do not have permission to perform this action',
        });
    }

    const files = await prisma.file.findMany({
        where: {
            authorId: currentUser.id,
        },
        orderBy: {
            createdAt: 'desc',
        },
        include: {
            views: true,
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

    return files.map((file) => ({
        ...file,
        size: {
            raw: file.size.toString(),
            formatted: filesize(file.size.toString()),
        },
        views: {
            total: file.views.length,
            today: file.views.filter((view) => {
                const now = new Date();

                return (
                    view.createdAt.getDate() === now.getDate() &&
                    view.createdAt.getMonth() === now.getMonth() &&
                    view.createdAt.getFullYear() === now.getFullYear()
                );
            }).length,
        },
        directUrl: `${protocol}://${domain}/u/${file.fileName}`,
        embedUrl: `${protocol}://${domain}/view/${file.fileName}`,
    }));
});
