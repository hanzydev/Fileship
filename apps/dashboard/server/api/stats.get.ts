import dayjs from 'dayjs';

export default defineEventHandler(async (event) => {
    adminOnly(event);

    const { dayAgo, stat, prevStat } = await getStatSnapshots();

    const [userCount, fileCount, storage, trashed, viewCount, prevViewCount, monthlyViews] =
        await Promise.all([
            prisma.user.count(),
            prisma.file.count(),
            prisma.file.aggregate({ _sum: { size: true } }),
            prisma.file.aggregate({ where: { deletedAt: { not: null } }, _sum: { size: true } }),
            prisma.view.count(),
            prisma.view.count({ where: { createdAt: { lte: dayAgo } } }),
            prisma.$queryRaw<{ month: string; count: number }[]>`
            SELECT to_char(date_trunc('month', "createdAt"), 'YYYY-MM') AS month, count(*)::int AS count
            FROM "View"
            WHERE "createdAt" >= date_trunc('month', now()) - interval '11 months'
            GROUP BY 1
        `,
        ]);

    const limitedUsers = (
        await prisma.user.findMany({
            select: {
                id: true,
                username: true,
                avatar: true,
                superAdmin: true,
                permissions: true,
                limits: true,
            },
        })
    )
        .map((user) => ({ user, limit: usableSpaceOf(user) }))
        .filter((entry) => entry.limit !== null);

    const usedByUser = await storageUsedByUsers(limitedUsers.map(({ user }) => user.id));

    const nearQuota = limitedUsers
        .map(({ user, limit }) => {
            const used = usedByUser.get(user.id) ?? '0';

            return {
                id: user.id,
                username: user.username,
                avatar: user.avatar,
                used,
                limit: limit!.toString(),
                ratio: Number(used) / Number(limit),
            };
        })
        .filter((entry) => entry.ratio >= 0.8)
        .sort((a, b) => b.ratio - a.ratio)
        .slice(0, 10)
        .map(({ ratio: _, ...entry }) => entry);

    const months = Array.from({ length: 12 }, (_, i) => dayjs().subtract(11 - i, 'month'));

    const viewsByMonth = {
        labels: months.map((month) => month.format('MMM YYYY')),
        months: months.map((month) => month.format('YYYY-MM')),
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
            size: formatSize(storage._sum.size ?? 0n),
            raw: (storage._sum.size ?? 0n).toString(),
            growth: calculateGrowthPercentage(storage._sum.size ?? 0n, prevStat?.storageUsed ?? 0),
            trashed: {
                raw: (trashed._sum.size ?? 0n).toString(),
                formatted: formatSize(trashed._sum.size ?? 0n),
            },
            byUser: storageUsedByUser
                .map((data) => ({
                    ...data,
                    formattedSize: formatSize(BigInt(data.size)),
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
        nearQuota,
    };
});
