import consola from 'consola';

export default defineTask({
    meta: {
        name: 'media:sync',
        description: 'Generates missing thumbnails',
    },
    async run() {
        let cursor: string | undefined;
        let processed = 0;

        while (true) {
            const files = await prisma.file.findMany({
                where: {
                    OR: [
                        { mimeType: { startsWith: 'image/' } },
                        { mimeType: { startsWith: 'video/' } },
                    ],
                },
                select: { id: true, fileName: true, mimeType: true },
                orderBy: { id: 'asc' },
                take: 100,
                ...(cursor && { skip: 1, cursor: { id: cursor } }),
            });

            if (!files.length) break;

            for (const file of files) {
                if (!canHaveThumbnail(file.mimeType) || hasThumbnail(file.id)) continue;

                await processMedia(file);
                processed++;
            }

            cursor = files.at(-1)!.id;
        }

        if (processed) consola.info(`Processed ${processed} media files.`);

        return { result: processed };
    },
});
