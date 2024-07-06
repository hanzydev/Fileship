import { isAdmin } from '~~/utils/user';

export default defineEventHandler((event) => {
    if (!isAdmin(event)) {
        throw createError({
            statusCode: 401,
            statusMessage: 'Unauthorized',
            message: 'You do not have permission to perform this action',
        });
    }

    return prisma.log.findMany({
        orderBy: {
            createdAt: 'desc',
        },
        select: {
            ip: true,
            action: true,
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
});
