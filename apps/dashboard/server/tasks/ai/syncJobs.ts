import consola from 'consola';
import { extname } from 'pathe';

import { AIJobStatus, AIJobType } from '~~/generated/prisma/enums';
import { AnyNull } from '~~/generated/prisma/internal/prismaNamespace';

export default defineTask({
    meta: {
        name: 'ai:syncJobs',
        description: 'Synchronizes missing AI jobs for files',
    },
    async run() {
        let enqueuedCount = 0;

        const processBatch = async (files: any[], type: AIJobType) => {
            const BATCH_SIZE = 50;
            for (let i = 0; i < files.length; i += BATCH_SIZE) {
                const chunk = files.slice(i, i + BATCH_SIZE);
                await Promise.all(
                    chunk.map(async (file) => {
                        await enqueueAIJob({
                            userId: file.authorId,
                            fileId: file.id,
                            type,
                        });
                        enqueuedCount++;
                    }),
                );
            }
        };

        const activeJobFilter = (type: AIJobType) => ({
            none: {
                type,
                status: { in: [AIJobStatus.Pending, AIJobStatus.Processing] },
            },
        });

        const imagesForClip = await prisma.file.findMany({
            where: {
                embedding: { equals: AnyNull },
                aiJobs: activeJobFilter(AIJobType.GenerateClipEmbedding),
            },
            select: { id: true, fileName: true, authorId: true },
        });

        const validImages = imagesForClip.filter((f) =>
            ai.IMAGE_EMBEDDING_SUPPORTED_EXTENSIONS.includes(extname(f.fileName)),
        );

        if (validImages.length) {
            consola.info(`Enqueuing ${validImages.length} images for CLIP...`);
            await processBatch(validImages, AIJobType.GenerateClipEmbedding);
        }

        const videosForClip = await prisma.file.findMany({
            where: {
                embedding: { equals: AnyNull },
                aiJobs: activeJobFilter(AIJobType.GenerateVideoEmbedding),
            },
            select: { id: true, fileName: true, authorId: true },
        });

        const validVideos = videosForClip.filter((f) =>
            ai.VIDEO_EMBEDDING_SUPPORTED_EXTENSIONS.includes(extname(f.fileName)),
        );

        if (validVideos.length) {
            consola.info(`Enqueuing ${validVideos.length} videos for CLIP...`);
            await processBatch(validVideos, AIJobType.GenerateVideoEmbedding);
        }

        const imagesForCaption = await prisma.file.findMany({
            where: {
                caption: null,
                mimeType: { startsWith: 'image/' },
                aiJobs: activeJobFilter(AIJobType.GenerateImageCaption),
            },
            select: { id: true, fileName: true, authorId: true },
        });

        const validCaptions = imagesForCaption.filter((f) =>
            ai.IMAGE_EMBEDDING_SUPPORTED_EXTENSIONS.includes(extname(f.fileName)),
        );

        if (validCaptions.length) {
            consola.info(`Enqueuing ${validCaptions.length} images for Caption...`);
            await processBatch(validCaptions, AIJobType.GenerateImageCaption);
        }

        const imagesForOcr = await prisma.file.findMany({
            where: {
                ocrText: null,
                mimeType: { startsWith: 'image/' },
                aiJobs: activeJobFilter(AIJobType.GenerateOcrText),
            },
            select: { id: true, fileName: true, authorId: true },
        });

        const validOcr = imagesForOcr.filter((f) =>
            ai.IMAGE_EMBEDDING_SUPPORTED_EXTENSIONS.includes(extname(f.fileName)),
        );

        if (validOcr.length) {
            consola.info(`Enqueuing ${validOcr.length} images for OCR...`);
            await processBatch(validOcr, AIJobType.GenerateOcrText);
        }

        const textForEmbedding = await prisma.file.findMany({
            where: {
                textEmbedding: { equals: AnyNull },
                OR: [
                    { ocrText: { not: null }, AND: { ocrText: { not: '' } } },
                    { mimeType: { startsWith: 'text/' } },
                ],
                aiJobs: activeJobFilter(AIJobType.GenerateTextEmbedding),
            },
            select: {
                id: true,
                fileName: true,
                authorId: true,
                ocrText: true,
                mimeType: true,
            },
        });

        const canEmbed = textForEmbedding.filter((f) => {
            const hasOcr = f.ocrText && f.ocrText.length > 0;
            const isText = ai.TEXT_EMBEDDING_SUPPORTED_EXTENSIONS.includes(extname(f.fileName));
            return hasOcr || isText;
        });

        if (canEmbed.length) {
            consola.info(`Enqueuing ${canEmbed.length} files for Text Embedding...`);
            await processBatch(canEmbed, AIJobType.GenerateTextEmbedding);
        }

        const filesForPii = await prisma.file.findMany({
            where: {
                aiJobs: {
                    none: {
                        type: AIJobType.DetectPII,
                    },
                },
                OR: [
                    { ocrText: { not: null }, AND: { ocrText: { not: '' } } },
                    { mimeType: { startsWith: 'text/' } },
                    { mimeType: { startsWith: 'image/' } },
                ],
            },
            select: {
                id: true,
                fileName: true,
                authorId: true,
                ocrText: true,
                mimeType: true,
            },
        });

        const canPii = filesForPii.filter((f) => {
            const hasOcr = f.ocrText && f.ocrText.length > 0;
            const isText = ai.TEXT_EMBEDDING_SUPPORTED_EXTENSIONS.includes(extname(f.fileName));
            const isImage = ai.IMAGE_EMBEDDING_SUPPORTED_EXTENSIONS.includes(extname(f.fileName));
            return hasOcr || isText || isImage;
        });

        if (canPii.length) {
            consola.info(`Enqueuing ${canPii.length} files for PII detection...`);
            await processBatch(canPii, AIJobType.DetectPII);
        }

        consola.success(`Job creation complete. Enqueued ${enqueuedCount} new jobs.`);
        return { result: 'success', enqueued: enqueuedCount };
    },
});
