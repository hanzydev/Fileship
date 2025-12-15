export default defineTask({
    meta: {
        name: 'db:deleteExpired',
        description: 'Deletes expired files',
    },
    async run() {
        const expired = await prisma.file.findMany({
            where: {
                expiresAt: {
                    lte: new Date(),
                },
            },
        });

        if (expired.length) {
            await prisma.file.deleteMany({
                where: {
                    id: {
                        in: expired.map((e) => e.id),
                    },
                },
            });

            expired.forEach((e) => {
                createLog(null, {
                    action: 'Delete File',
                    message: `Deleted file ${e.fileName} due to expiration`,
                    system: true,
                });

                sendToUser(e.authorId, `file:delete`, e.id);
            });
        }

        return { result: 'success' };
    },
});
