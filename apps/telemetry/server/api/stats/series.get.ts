import { z } from 'zod';

const querySchema = z.object({
    range: z.enum(STAT_RANGES).default('90d'),
});

export default defineEventHandler(async (event) => {
    adminOnly(event);

    const query = await getValidatedQuery(event, querySchema.safeParse);

    if (!query.success) {
        throw createError({
            status: 400,
            message: 'Invalid query',
        });
    }

    const days = STAT_RANGE_DAYS[query.data.range];

    const stats = await prisma.dailyStat.findMany({
        where: days ? { day: { gte: addDays(startOfDay(new Date()), 1 - days) } } : {},
        orderBy: { day: 'asc' },
        select: {
            day: true,
            instances: true,
            users: true,
            files: true,
            storage: true,
            links: true,
            versions: true,
        },
    });

    return stats.map((stat) => ({
        ...stat,
        day: toDateString(stat.day),
        storage: Number(stat.storage),
        versions: stat.versions as Record<string, number>,
    }));
});
