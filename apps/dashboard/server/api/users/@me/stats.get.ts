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

    const [users, notes, folders, currentFiles, storage, currentViews, prevViews, ...categories] =
        await Promise.all([
            isAdmin(currentUser) ? prisma.user.count() : 0,
            prisma.note.count({ where: { authorId: currentUser.id } }),
            prisma.folder.count({ where: { authorId: currentUser.id } }),
            prisma.file.count({ where: { authorId: currentUser.id } }),
            prisma.file.aggregate({ where: { authorId: currentUser.id }, _sum: { size: true } }),
            prisma.view.count({ where: { ownerId: currentUser.id } }),
            prisma.view.count({ where: { ownerId: currentUser.id, createdAt: { lte: dayAgo } } }),
            ...FILE_CATEGORY_NAMES.map((category) =>
                prisma.file.aggregate({
                    where: { AND: [{ authorId: currentUser.id }, fileCategoryWhere(category)] },
                    _count: true,
                    _sum: { size: true },
                }),
            ),
        ]);

    const currentStorageUsed = storage._sum.size ?? 0n;

    return {
        users: {
            count: users,
            growth: calculateGrowthPercentage(users, prevStat?.users ?? 0),
        },
        notes: { count: notes },
        folders: { count: folders },
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
        byType: FILE_CATEGORY_NAMES.map((type, index) => {
            const size = categories[index]!._sum.size ?? 0n;

            return {
                type,
                count: categories[index]!._count,
                size: size.toString(),
                formatted: filesize(size),
            };
        }).filter((category) => category.count),
    };
});
