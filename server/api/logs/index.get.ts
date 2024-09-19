import { isAdmin } from '~~/utils/user';

export default defineEventHandler(async (event) => {
    if (!isAdmin(event)) {
        throw createError({
            statusCode: 401,
            statusMessage: 'Unauthorized',
            message: 'You do not have permission to perform this action',
        });
    }

    const logs = await prisma.log.findMany({
        orderBy: {
            createdAt: 'desc',
        },
        select: {
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
                    log.user &&
                    self.findIndex((t) => t.user?.id === log.user?.id) ===
                        index,
            )
            .map((log) => log.user!),
    };
});
