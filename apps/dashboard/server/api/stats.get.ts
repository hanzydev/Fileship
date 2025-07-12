import { filesize } from 'filesize';

export default defineEventHandler(async (event) => {
    adminOnly(event);

    const [stat, prevStat] = await prisma.stat.findMany({
        where: {
            createdAt: {
                in: (
                    await prisma.stat.groupBy({
                        by: ['createdAt'],
                        _max: {
                            createdAt: true,
                        },
                        orderBy: {
                            _max: {
                                createdAt: 'desc',
                            },
                        },
                    })
                ).map((group) => group._max.createdAt!),
            },
        },
        orderBy: {
            createdAt: 'desc',
        },
        take: 2,
    });

    const now = new Date();

    const currentViews = (stat?.views as number[]) || [];
    const prevViews = (prevStat?.views as number[]) || [];

    const viewsByMonth = currentViews.reduce(
        (acc, createdTimestamp) => {
            const createdAt = new Date(createdTimestamp);

            if (createdAt.getFullYear() === now.getFullYear()) {
                acc.data[createdAt.getMonth()]!++;
            }

            return acc;
        },
        {
            labels: [
                'January',
                'February',
                'March',
                'April',
                'May',
                'June',
                'July',
                'August',
                'September',
                'October',
                'November',
                'December',
            ],
            data: Array.from({ length: 12 }, () => 0),
        },
    );

    const topUploaders = (stat?.filesByUser as { userId: string; count: number }[]) ?? [];

    const storageUsedByUser = (stat?.storageUsedByUser as { userId: string; size: string }[]) ?? [];

    const users = await prisma.user.findMany({
        where: {
            id: {
                in: [
                    ...topUploaders.map((user) => user.userId),
                    ...storageUsedByUser.map((user) => user.userId),
                ].filter((userId, index, self) => self.indexOf(userId) === index),
            },
        },
        select: {
            id: true,
            username: true,
            avatar: true,
        },
    });

    return {
        users: {
            count: stat?.users ?? 0,
            growth: calculateGrowthPercentage(stat?.users ?? 0, prevStat?.users ?? 0),
            all: users,
        },
        files: {
            count: stat?.files ?? 0,
            growth: calculateGrowthPercentage(stat?.files ?? 0, prevStat?.files ?? 0),
        },
        storageUsed: {
            size: filesize(stat?.storageUsed?.toString() ?? 0),
            growth: calculateGrowthPercentage(stat?.storageUsed ?? 0, prevStat?.storageUsed ?? 0),
            byUser: storageUsedByUser
                .map((data) => ({
                    ...data,
                    formattedSize: filesize(data.size),
                }))
                .filter((u) => users.find((user) => user.id === u.userId)),
        },
        views: {
            count: currentViews.length ?? 0,
            growth: calculateGrowthPercentage(currentViews.length ?? 0, prevViews.length ?? 0),
            byMonth: viewsByMonth,
        },
        topTypes: (stat?.types ?? []) as { type: string; count: number }[],
        topUploaders: topUploaders.filter((u) => users.find((user) => user.id === u.userId)),
    };
});
