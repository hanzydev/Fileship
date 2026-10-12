import consola from 'consola';

export default defineTask({
    meta: {
        name: 'media:sync',
        description: 'Generates missing thumbnails and media metadata',
    },
    async run() {
        let cursor: string | undefined;
        let processed = 0;

        while (true) {
            const files = await prisma.file.findMany({
                where: {
                    OR: ['image/', 'video/', 'audio/'].map((type) => ({
                        mimeType: { startsWith: type },
                    })),
                },
                select: {
                    id: true,
                    fileName: true,
                    mimeType: true,
                    metadata: true,
                    storageId: true,
                },
                orderBy: { id: 'asc' },
                take: 100,
                ...(cursor && { skip: 1, cursor: { id: cursor } }),
            });

            if (!files.length) break;

            for (const file of files) {
                if (file.metadata && (!canHaveThumbnail(file.mimeType) || hasThumbnail(file.id)))
                    continue;

                await withLocalFile(file.storageId, uploadKey(file.fileName), (filePath) =>
                    processMedia(file, filePath),
                ).catch(() => null);
                processed++;
            }

            cursor = files.at(-1)!.id;
        }

        if (processed) consola.info(`Processed ${processed} media files.`);

        return { result: processed };
    },
});
