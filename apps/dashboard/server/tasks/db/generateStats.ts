import consola from 'consola';
import dayjs from 'dayjs';

export default defineTask({
    meta: {
        name: 'db:generateStats',
        description: 'Generates statistics',
    },
    async run() {
        const [users, storage, filesCount, byUser, viewsByOwner, types] = await Promise.all([
            prisma.user.count(),
            prisma.file.aggregate({ _sum: { size: true } }),
            prisma.file.count({ where: { deletedAt: null } }),
            prisma.file.groupBy({ by: ['authorId'], _count: true, _sum: { size: true } }),
            prisma.view.groupBy({
                by: ['ownerId'],
                _count: true,
                where: { ownerId: { not: null } },
            }),
            prisma.file.groupBy({ by: ['mimeType'], _count: true, where: { deletedAt: null } }),
        ]);

        await prisma.stat.create({
            data: {
                users,
                storageUsed: storage._sum.size ?? 0n,
                storageUsedByUser: byUser
                    .map((u) => ({ userId: u.authorId, size: (u._sum.size ?? 0n).toString() }))
                    .sort((a, b) => +b.size - +a.size),
                views: [],
                viewsByUser: viewsByOwner
                    .map((v) => ({ userId: v.ownerId!, count: v._count }))
                    .sort((a, b) => b.count - a.count),
                files: filesCount,
                filesByUser: byUser
                    .map((u) => ({ userId: u.authorId, count: u._count }))
                    .sort((a, b) => b.count - a.count),
                types: types
                    .map((t) => ({ type: t.mimeType, count: t._count }))
                    .sort((a, b) => b.count - a.count),
            },
        });

        await prisma.stat.deleteMany({
            where: { createdAt: { lt: dayjs().subtract(30, 'day').toDate() } },
        });

        consola.success(`${dayjs().format('YYYY-MM-DD HH:mm:ss')} - Stats generated.`);

        return { result: 'success' };
    },
});
