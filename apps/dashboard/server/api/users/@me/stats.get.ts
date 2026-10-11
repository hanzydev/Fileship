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

    const [users, currentFiles, storage, currentViews, prevViews] = await Promise.all([
        prisma.user.count(),
        prisma.file.count({ where: { authorId: currentUser.id } }),
        prisma.file.aggregate({ where: { authorId: currentUser.id }, _sum: { size: true } }),
        prisma.view.count({ where: { ownerId: currentUser.id } }),
        prisma.view.count({ where: { ownerId: currentUser.id, createdAt: { lte: dayAgo } } }),
    ]);

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
        },
    };
});
