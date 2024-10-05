export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;

    const urls = await prisma.url.findMany({
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

    return urls.map((url) => ({
        ...url,
        views: {
            total: url.views.length,
            today: url.views.filter((view) => {
                const now = new Date();

                return (
                    view.createdAt.getDate() === now.getDate() &&
                    view.createdAt.getMonth() === now.getMonth() &&
                    view.createdAt.getFullYear() === now.getFullYear()
                );
            }).length,
        },
        url: buildPublicUrl(event, currentUser.domains, `/link/${url.vanity}`),
    }));
});
