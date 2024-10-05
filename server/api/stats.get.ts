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

    const topUploadersRaw =
        (stat?.filesByUser as { userId: string; count: number }[]) ?? [];

    const users = await prisma.user.findMany({
        where: {
            id: {
                in: topUploadersRaw.map((user) => user.userId),
            },
        },
        select: {
            id: true,
            username: true,
            avatar: true,
        },
    });

    const topUploaders = topUploadersRaw.reduce(
        (acc, topUploader) => {
            const user = users.find((user) => user.id === topUploader.userId);

            if (user) {
                acc.push({
                    user,
                    count: topUploader.count,
                });
            }

            return acc;
        },
        [] as {
            user: { id: string; username: string; avatar: string | null };
            count: number;
        }[],
    );

    return {
        users: {
            count: stat?.users ?? 0,
            growth: calculateGrowthPercentage(
                stat?.users ?? 0,
                prevStat?.users ?? 0,
            ),
        },
        files: {
            count: stat?.files ?? 0,
            growth: calculateGrowthPercentage(
                stat?.files ?? 0,
                prevStat?.files ?? 0,
            ),
        },
        storageUsed: {
            size: filesize(stat?.storageUsed?.toString() ?? 0),
            growth: calculateGrowthPercentage(
                stat?.storageUsed ?? 0,
                prevStat?.storageUsed ?? 0,
            ),
        },
        views: {
            count: currentViews.length ?? 0,
            growth: calculateGrowthPercentage(
                currentViews.length ?? 0,
                prevViews.length ?? 0,
            ),
            byMonth: viewsByMonth,
        },
        topTypes: (stat?.types ?? []) as { type: string; count: number }[],
        topUploaders,
    };
});
