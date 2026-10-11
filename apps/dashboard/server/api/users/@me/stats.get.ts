import dayjs from 'dayjs';
import { filesize } from 'filesize';

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;

    const { dayAgo, prevStat } = await getStatSnapshots();

    const prevFiles =
        (prevStat?.filesByUser as { userId: string; count: number }[])?.find(
            (file) => file.userId === currentUser.id,
        )?.count ?? 0;

    const prevStorageUsed =
        (prevStat?.storageUsedByUser as { userId: string; size: string }[])?.find(
            (storage) => storage.userId === currentUser.id,
        )?.size ?? '0';

    const [users, currentFiles, storage, currentViews, prevViews, dailyViews, ...categorySizes] =
        await Promise.all([
            prisma.user.count(),
            prisma.file.count({ where: { authorId: currentUser.id } }),
            prisma.file.aggregate({ where: { authorId: currentUser.id }, _sum: { size: true } }),
            prisma.view.count({ where: { ownerId: currentUser.id } }),
            prisma.view.count({ where: { ownerId: currentUser.id, createdAt: { lte: dayAgo } } }),
            prisma.$queryRaw<{ day: string; count: number }[]>`
                SELECT to_char(date_trunc('day', "createdAt"), 'YYYY-MM-DD') AS day, count(*)::int AS count
                FROM "View"
                WHERE "ownerId" = ${currentUser.id}
                AND "createdAt" >= date_trunc('day', now()) - interval '29 days'
                GROUP BY 1
            `,
            ...FILE_CATEGORY_NAMES.map((category) =>
                prisma.file.aggregate({
                    where: { AND: [{ authorId: currentUser.id }, fileCategoryWhere(category)] },
                    _sum: { size: true },
                }),
            ),
        ]);

    const days = Array.from({ length: 30 }, (_, i) => dayjs().subtract(29 - i, 'day'));

    const currentStorageUsed = storage._sum.size ?? 0n;

    return {
        users: {
            count: users,
            growth: calculateGrowthPercentage(users, prevStat?.users ?? 0),
        },
        files: {
            count: currentFiles,
            growth: calculateGrowthPercentage(currentFiles, prevFiles),
        },
        storageUsed: {
            size: filesize(currentStorageUsed),
            growth: calculateGrowthPercentage(currentStorageUsed, BigInt(prevStorageUsed)),
        },
        views: {
            count: currentViews,
            growth: calculateGrowthPercentage(currentViews, prevViews),
            daily: {
                labels: days.map((day) => day.format('MMM D')),
                data: days.map(
                    (day) =>
                        dailyViews.find((view) => view.day === day.format('YYYY-MM-DD'))?.count ??
                        0,
                ),
            },
        },
        storageByType: FILE_CATEGORY_NAMES.map((category, index) => {
            const size = categorySizes[index]!._sum.size ?? 0n;
            return { type: category, size: size.toString(), formatted: filesize(size) };
        }).filter((category) => category.size !== '0'),
    };
});
