export default defineTask({
    meta: {
        name: 'db:deleteExpired',
        description: 'Deletes expired files and links',
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
                deletedAt: true,
                name: true,
                mimeType: true,
                size: true,
                password: true,
                piiDetected: true,
                aiAnalyzedAt: true,
                folder: { select: { public: true } },
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

            if (!file.deletedAt)
                emitFilesDeleted(null, file.authorId, [{ ...file, expiresAt: null }], 'expired');

            sendToUser(file.authorId, 'file:delete', file.id);
            if (file.deletedAt) sendToUser(file.authorId, 'trash:update', null);
        }

        const expiredLinks = await prisma.link.findMany({
            where: { expiresAt: { lte: new Date() } },
            select: { id: true, code: true, authorId: true },
        });

        for (const link of expiredLinks) {
            const { count } = await prisma.link.deleteMany({
                where: { id: link.id, expiresAt: { lte: new Date() } },
            });

            if (!count) continue;

            await createLog(null, {
                action: 'Delete Link',
                message: `Deleted link ${link.code} due to expiration`,
                system: true,
            });

            sendToUser(link.authorId, 'link:delete', link.id);
        }

        return { result: 'success' };
    },
});
