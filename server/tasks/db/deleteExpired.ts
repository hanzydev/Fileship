export default defineTask({
    meta: {
        name: 'db:deleteExpired',
        description: 'Deletes expired files, codes and urls',
    },
    async run() {
        const keys = ['File', 'Code', 'URL'];

        for (const key of keys) {
            const model = prisma[key.toLowerCase() as never] as any;

            const expired = (await model.findMany({
                where: {
                    expiresAt: {
                        lte: new Date(),
                    },
                },
            })) as any[];

            if (expired.length) {
                await model.deleteMany({
                    where: {
                        id: {
                            in: expired.map((e) => e.id),
                        },
                    },
                });

                expired.forEach((e) => {
                    createLog(null, {
                        action: `Delete ${key}`,
                        message: `Deleted ${key.toLowerCase()} ${e.vanity || e.title || e.fileName} due to expiration`,
                        system: true,
                    });

                    sendToUser(e.authorId, `delete:${key.toLowerCase()}`, e.id);
                });
            }
        }

        return { result: 'success' };
    },
});
