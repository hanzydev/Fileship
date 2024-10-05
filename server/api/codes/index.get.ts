export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;

    const codes = await prisma.code.findMany({
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

    return codes.map((code) => ({
        ...code,
        views: {
            total: code.views.length,
            today: code.views.filter((view) => {
                const now = new Date();

                return (
                    view.createdAt.getDate() === now.getDate() &&
                    view.createdAt.getMonth() === now.getMonth() &&
                    view.createdAt.getFullYear() === now.getFullYear()
                );
            }).length,
        },
        url: buildPublicUrl(event, currentUser.domains, `/code/${code.id}`),
    }));
});
