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
        directUrl: buildPublicUrl(
            event,
            currentUser.domains,
            `/u/${file.fileName}`,
        ),
        embedUrl: buildPublicUrl(
            event,
            currentUser.domains,
            `/view/${file.fileName}`,
        ),
    }));
});
