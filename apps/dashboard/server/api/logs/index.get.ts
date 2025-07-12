export default defineEventHandler(async (event) => {
    adminOnly(event);

    const logs = await prisma.log.findMany({
        orderBy: {
            createdAt: 'desc',
        },
        select: {
            id: true,
            ip: true,
            action: true,
            userId: true,
            user: {
                select: {
                    id: true,
                    username: true,
                    avatar: true,
                },
            },
            message: true,
            system: true,
            createdAt: true,
        },
    });

    return {
        logs: logs.map((log) => ({
            ...log,
            user: undefined,
        })),
        users: logs
            .filter(
                (log, index, self) =>
                    log.user && self.findIndex((t) => t.user?.id === log.user?.id) === index,
            )
            .map((log) => log.user!),
    };
});
