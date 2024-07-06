import { filesize } from 'filesize';

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;

    if (!currentUser) {
        throw createError({
            statusCode: 401,
            statusMessage: 'Unauthorized',
            message: 'You do not have permission to perform this action',
        });
    }

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

    const currentFiles =
        (stat?.filesByUser as { userId: string; count: number }[])?.find(
            (file) => file.userId === currentUser.id,
        )?.count ?? 0;

    const prevFiles =
        (prevStat?.filesByUser as { userId: string; count: number }[])?.find(
            (file) => file.userId === currentUser.id,
        )?.count ?? 0;

    const currentStorageUsed =
        (stat?.storageUsedByUser as { userId: string; size: string }[])?.find(
            (storage) => storage.userId === currentUser.id,
        )?.size ?? '0';

    const prevStorageUsed =
        (
            prevStat?.storageUsedByUser as { userId: string; size: string }[]
        )?.find((storage) => storage.userId === currentUser.id)?.size ?? '0';

    const currentViews =
        (stat?.viewsByUser as { userId: string; count: number }[])?.find(
            (view) => view.userId === currentUser.id,
        )?.count ?? 0;

    const prevViews =
        (prevStat?.viewsByUser as { userId: string; count: number }[])?.find(
            (view) => view.userId === currentUser.id,
        )?.count ?? 0;

    return {
        users: {
            count: stat?.users ?? 0,
            growth: calculateGrowthPercentage(
                stat?.users ?? 0,
                prevStat?.users ?? 0,
            ),
        },
        files: {
            count: currentFiles,
            growth: calculateGrowthPercentage(currentFiles, prevFiles),
        },
        storageUsed: {
            size: filesize(currentStorageUsed),
            growth: calculateGrowthPercentage(
                BigInt(currentStorageUsed),
                BigInt(prevStorageUsed),
            ),
        },
        views: {
            count: currentViews,
            growth: calculateGrowthPercentage(currentViews, prevViews),
        },
    };
});
