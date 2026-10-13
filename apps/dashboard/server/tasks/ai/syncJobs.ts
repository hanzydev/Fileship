import consola from 'consola';

import type { Prisma } from '~~/generated/prisma/client';

const BATCH_SIZE = 500;

export default defineTask({
    meta: {
        name: 'ai:syncJobs',
        description: 'Queues analysis jobs for files that are missing AI data',
    },
    async run({ payload }) {
        if (!(await isAiEnabled())) {
            consola.warn('AI is disabled, AI jobs are not synchronized.');
            return { result: 'skipped' };
        }

        const preset = await getAIPreset();
        const userId = payload?.userId as string | undefined;
        let cursor: string | undefined;
        let enqueued = 0;

        const where: Prisma.FileWhereInput = {
            deletedAt: null,
            ...(userId ? { authorId: userId } : {}),
            author: { NOT: { aiSettings: { path: ['enabled'], equals: false } } },
            aiJobs: { none: {} },
            OR: [
                { aiAnalyzedAt: null },
                { embeddingModel: null },
                { embeddingModel: { not: preset.image.key } },
                { textEmbeddingModel: null },
                { textEmbeddingModel: { not: preset.text.key } },
            ],
        };

        while (true) {
            const files = await prisma.file.findMany({
                where,
                select: {
                    id: true,
                    authorId: true,
                    fileName: true,
                    aiAnalyzedAt: true,
                    embeddingModel: true,
                    textEmbeddingModel: true,
                },
                orderBy: { id: 'asc' },
                take: BATCH_SIZE,
                ...(cursor ? { cursor: { id: cursor }, skip: 1 } : {}),
            });

            for (const file of files) {
                const kind = getAIFileKind(file.fileName);
                if (!kind) continue;

                const needsImage = kind !== 'text' && file.embeddingModel !== preset.image.key;
                const needsText = kind !== 'video' && file.textEmbeddingModel !== preset.text.key;

                if (!file.aiAnalyzedAt || needsImage || needsText) {
                    if (await enqueueFileAnalysis(file)) enqueued++;
                }
            }

            if (files.length < BATCH_SIZE) break;
            cursor = files.at(-1)!.id;
        }

        if (enqueued) consola.info(`Queued ${enqueued} files for AI analysis.`);
        return { result: 'success', enqueued };
    },
});
