import dayjs from 'dayjs';
import { filesize } from 'filesize';

export default defineEventHandler(async (event) => {
    adminOnly(event);

    const { dayAgo, stat, prevStat } = await getStatSnapshots();

    const [userCount, fileCount, storage, viewCount, prevViewCount, monthlyViews] =
        await Promise.all([
            prisma.user.count(),
            prisma.file.count(),
            prisma.file.aggregate({ _sum: { size: true } }),
            prisma.view.count(),
            prisma.view.count({ where: { createdAt: { lte: dayAgo } } }),
            prisma.$queryRaw<{ month: string; count: number }[]>`
            SELECT to_char(date_trunc('month', "createdAt"), 'YYYY-MM') AS month, count(*)::int AS count
            FROM "View"
            WHERE "createdAt" >= date_trunc('month', now()) - interval '11 months'
            GROUP BY 1
        `,
        ]);

    const months = Array.from({ length: 12 }, (_, i) => dayjs().subtract(11 - i, 'month'));

    const viewsByMonth = {
        labels: months.map((month) => month.format('MMM YYYY')),
        data: months.map(
            (month) => monthlyViews.find((m) => m.month === month.format('YYYY-MM'))?.count || 0,
        ),
    };

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
            count: userCount,
            growth: calculateGrowthPercentage(userCount, prevStat?.users ?? 0),
            all: users,
        },
        files: {
            count: fileCount,
            growth: calculateGrowthPercentage(fileCount, prevStat?.files ?? 0),
        },
        storageUsed: {
            size: filesize(storage._sum.size ?? 0n),
            growth: calculateGrowthPercentage(storage._sum.size ?? 0n, prevStat?.storageUsed ?? 0),
            byUser: storageUsedByUser
                .map((data) => ({
                    ...data,
                    formattedSize: filesize(BigInt(data.size)),
                }))
                .filter((u) => users.find((user) => user.id === u.userId)),
        },
        views: {
            count: viewCount,
            growth: calculateGrowthPercentage(viewCount, prevViewCount),
            byMonth: viewsByMonth,
        },
        topTypes: (stat?.types ?? []) as { type: string; count: number }[],
        topUploaders: topUploaders.filter((u) => users.find((user) => user.id === u.userId)),
    };
});
