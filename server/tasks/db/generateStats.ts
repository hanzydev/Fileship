import consola from 'consola';
import dayjs from 'dayjs';

export default defineTask({
    meta: {
        name: 'db:generateStats',
        description: 'Generates statistics',
    },
    async run() {
        const users = await prisma.user.count();
        const files = await prisma.file.findMany({
            include: { views: true },
        });

        const storageUsedByUser = files
            .reduce(
                (acc, file) => {
                    const userIndex = acc.findIndex(({ userId }) => userId === file.authorId);

                    if (userIndex === -1) {
                        acc.push({
                            userId: file.authorId,
                            size: file.size.toString(),
                        });
                    } else {
                        acc[userIndex]!.size = (
                            BigInt(acc[userIndex]!.size) + file.size
                        ).toString();
                    }

                    return acc;
                },
                [] as { userId: string; size: string }[],
            )
            .sort((a, b) => +b.size - +a.size);

        const views = files.reduce((acc, file) => {
            file.views.forEach((view) => acc.push(view.createdAt.getTime()));
            return acc;
        }, [] as number[]);

        const viewsByUser = files
            .reduce(
                (acc, file) => {
                    const userIndex = acc.findIndex(({ userId }) => userId === file.authorId);

                    if (userIndex === -1) {
                        acc.push({
                            userId: file.authorId,
                            count: file.views.length,
                        });
                    } else {
                        acc[userIndex]!.count += file.views.length;
                    }

                    return acc;
                },
                [] as { userId: string; count: number }[],
            )
            .sort((a, b) => b.count - a.count);

        const filesByUser = files
            .reduce(
                (acc, file) => {
                    const userIndex = acc.findIndex(({ userId }) => userId === file.authorId);

                    if (userIndex === -1) {
                        acc.push({
                            userId: file.authorId,
                            count: 1,
                        });
                    } else {
                        acc[userIndex]!.count++;
                    }

                    return acc;
                },
                [] as { userId: string; count: number }[],
            )
            .sort((a, b) => b.count - a.count);

        const types = files
            .reduce(
                (acc, file) => {
                    const typeIndex = acc.findIndex(({ type }) => type === file.mimeType);

                    if (typeIndex === -1) {
                        acc.push({
                            type: file.mimeType,
                            count: 1,
                        });
                    } else {
                        acc[typeIndex]!.count++;
                    }

                    return acc;
                },
                [] as { type: string; count: number }[],
            )
            .sort((a, b) => b.count - a.count);

        await prisma.stat.create({
            data: {
                users,
                storageUsed: files.reduce((acc, file) => acc + file.size, 0n),
                storageUsedByUser,
                views,
                viewsByUser,
                files: files.length,
                filesByUser,
                types,
            },
        });

        consola.success(`${dayjs().format('YYYY-MM-DD HH:mm:ss')} - Stats generated.`);

        return { result: 'success' };
    },
});
