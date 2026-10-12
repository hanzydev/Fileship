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
            select: {
                id: true,
                fileName: true,
                storageId: true,
                authorId: true,
            },
        });

        for (const file of expired) {
            const { count } = await prisma.file.deleteMany({
                where: { id: file.id, expiresAt: { lte: new Date() } },
            });

            if (!count) continue;

            await removeFileData([file]);

            await createLog(null, {
                action: 'Delete File',
                message: `Deleted file ${file.fileName} due to expiration`,
                system: true,
            });

            sendToUser(file.authorId, 'file:delete', file.id);
        }

        return { result: 'success' };
    },
});
