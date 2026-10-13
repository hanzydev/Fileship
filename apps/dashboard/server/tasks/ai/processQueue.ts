import { randomUUID } from 'node:crypto';

import consola from 'consola';
import dayjs from 'dayjs';
import { extname } from 'pathe';

import { AIJobStatus, AIJobType } from '#shared/prisma/enums';
import { DbNull } from '~~/generated/prisma/internal/prismaNamespace';

const WORKER_ID = randomUUID();

export default defineTask({
    meta: {
        name: 'ai:processQueue',
        description: 'Processes pending AI jobs',
    },
    async run() {
        const workerId = `FILESHIP-AI-RUNNER-${WORKER_ID}`;

        const envConcurrency = (await getSettings()).aiQueueConcurrency;
        const concurrency = Math.max(
            1,
            Math.min(8, envConcurrency > 0 ? Math.floor(envConcurrency) : 1),
        );

        const jobs = [] as NonNullable<Awaited<ReturnType<typeof dequeueAIJob>>>[];
        for (let i = 0; i < concurrency; i++) {
            const job = await dequeueAIJob({ workerId });
            if (!job) break;
            jobs.push(job);
        }

        if (!jobs.length) return { result: 'empty' };

        const processOne = async (job: (typeof jobs)[number]) => {
            const start = Date.now();

            try {
                const user = await prisma.user.findUnique({
                    where: { id: job.userId },
                    select: { aiSettings: true },
                });

                const aiEnabled = (user?.aiSettings as any)?.enabled ?? true;
                if (!aiEnabled) {
                    await completeAIJob(job.id);
                    return { id: job.id, result: 'disabled' };
                }

                if (!job.fileId) throw new Error('Missing fileId');

                const file = await prisma.file.findUnique({
                    where: { id: job.fileId, deletedAt: null },
                    select: {
                        id: true,
                        fileName: true,
                        storageId: true,
                        authorId: true,
                        ocrText: true,
                        caption: true,
                        piiOverride: true,
                    },
                });

                if (!file) throw new Error('File not found');

                const withFile = <T>(fn: (filePath: string) => Promise<T>) =>
                    withLocalFile(file.storageId, uploadKey(file.fileName), fn);

                const isImage = ai.IMAGE_EMBEDDING_SUPPORTED_EXTENSIONS.includes(
                    extname(file.fileName),
                );

                if (
                    job.type === AIJobType.GenerateClipEmbedding ||
                    job.type === AIJobType.GenerateVideoEmbedding
                ) {
                    const embedding = await withFile((filePath) =>
                        job.type === AIJobType.GenerateClipEmbedding
                            ? ai.createImageEmbedding(filePath)
                            : ai.createVideoEmbedding({ filePath }),
                    );

                    await prisma.file.update({ where: { id: file.id }, data: { embedding } });
                    await upsertFileSearchEntry(file.id);
                }

                if (job.type === AIJobType.GenerateOcrText) {
                    const ocrText = await withFile((filePath) => ai.performOcrOnImage(filePath));

                    await prisma.file.update({ where: { id: file.id }, data: { ocrText } });
                    await upsertFileSearchEntry(file.id);

                    for (const type of [AIJobType.GenerateTextEmbedding, AIJobType.DetectPII])
                        await enqueueAIJob({ userId: job.userId, fileId: file.id, type });
                }

                if (job.type === AIJobType.GenerateImageCaption) {
                    const caption = await withFile((filePath) => ai.generateImageCaption(filePath));

                    await prisma.file.update({ where: { id: file.id }, data: { caption } });
                    await upsertFileSearchEntry(file.id);

                    await enqueueAIJob({
                        userId: job.userId,
                        fileId: file.id,
                        type: AIJobType.GenerateTextEmbedding,
                    });

                    sendToUser(file.authorId, 'file:update', { id: file.id, caption });
                }

                if (job.type === AIJobType.GenerateTextEmbedding) {
                    const text = isImage
                        ? [file.caption, file.ocrText].filter(Boolean).join('\n')
                        : ai.TEXT_EMBEDDING_SUPPORTED_EXTENSIONS.includes(extname(file.fileName))
                          ? await withFile((filePath) => readUtf8Snippet(filePath, 120_000))
                          : '';

                    const textEmbedding = await ai.createSemanticTextEmbedding(text);

                    await prisma.file.update({
                        where: { id: file.id },
                        data: { textEmbedding: textEmbedding ?? DbNull },
                    });
                    await upsertFileSearchEntry(file.id);
                }

                if (job.type === AIJobType.DetectPII && file.piiOverride === null) {
                    const { piiDetected, reasons } = await withFile((filePath) =>
                        ai.detectPII(filePath, file.ocrText),
                    );

                    await prisma.file.update({
                        where: { id: file.id },
                        data: { piiDetected, piiReasons: reasons },
                    });

                    sendToUser(file.authorId, 'file:update', {
                        id: file.id,
                        piiDetected,
                        piiReasons: reasons,
                    });
                }

                await completeAIJob(job.id);

                consola.success(
                    `${dayjs().format('YYYY-MM-DD HH:mm:ss')} - Processed AI job ${job.id} (${job.type}) in ${Date.now() - start}ms`,
                );

                return { id: job.id, result: 'success' };
            } catch (error: any) {
                const message = error?.message || 'Unknown error';
                await failAIJob(job.id, message);

                consola.error(
                    `${dayjs().format('YYYY-MM-DD HH:mm:ss')} - AI job ${job.id} failed: ${message}`,
                );

                console.log(error);

                return { id: job.id, result: 'failed' };
            } finally {
                await prisma.aIJob.updateMany({
                    where: { id: job.id, status: AIJobStatus.Processing, lockedBy: workerId },
                    data: { lockedAt: null, lockedBy: null },
                });
            }
        };

        const results = await Promise.allSettled(jobs.map((j) => processOne(j)));
        const processed = results.length;
        const failed = results.filter(
            (r) =>
                r.status === 'rejected' ||
                (r.status === 'fulfilled' && r.value.result === 'failed'),
        ).length;

        return { result: failed ? 'partial' : 'success', processed, failed, concurrency };
    },
});
