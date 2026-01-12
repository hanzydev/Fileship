import os from 'node:os';

import consola from 'consola';
import dayjs from 'dayjs';
import fluentFfmpeg from 'fluent-ffmpeg';
import { extname, join } from 'pathe';

import ffmpeg from '@ffmpeg-installer/ffmpeg';
import { insert, remove } from '@orama/orama';

import { AIJobStatus, AIJobType } from '~~/generated/prisma/enums';

fluentFfmpeg.setFfmpegPath(ffmpeg.path);

export default defineTask({
    meta: {
        name: 'ai:processQueue',
        description: 'Processes pending AI jobs',
    },
    async run() {
        const workerId = `FILESHIP-AI-RUNNER-${process.pid}`;

        const envConcurrency = +(process.env.AI_QUEUE_CONCURRENCY ?? 0);
        const defaultConcurrency = Math.max(1, os.cpus().length - 1);
        const concurrency = Math.max(
            1,
            Math.min(8, envConcurrency > 0 ? envConcurrency : defaultConcurrency),
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

                if (job.type === AIJobType.GenerateClipEmbedding) {
                    if (!job.fileId) throw new Error('Missing fileId');

                    const file = await prisma.file.findUnique({
                        where: { id: job.fileId },
                        select: {
                            id: true,
                            fileName: true,
                            mimeType: true,
                            authorId: true,
                            textEmbedding: true,
                        },
                    });

                    if (!file) throw new Error('File not found');

                    const filePath = join(dataDirectory, 'uploads', file.fileName);
                    const embedding = await ai.createImageEmbedding(filePath);

                    await prisma.file.update({
                        where: { id: file.id },
                        data: { embedding },
                    });

                    try {
                        await remove(fileSearchDb, file.id);
                    } catch {
                        //
                    }

                    await insert(fileSearchDb, {
                        id: file.id,
                        fileName: file.fileName,
                        mimeType: file.mimeType,
                        embedding,
                        textEmbedding: (file.textEmbedding as number[]) || undefined,
                    });

                    sendToUser(file.authorId, 'file:update', { id: file.id, embedding });
                }

                if (job.type === AIJobType.GenerateVideoEmbedding) {
                    if (!job.fileId) throw new Error('Missing fileId');

                    const file = await prisma.file.findUnique({
                        where: { id: job.fileId },
                        select: {
                            id: true,
                            fileName: true,
                            mimeType: true,
                            authorId: true,
                            textEmbedding: true,
                        },
                    });

                    if (!file) throw new Error('File not found');

                    const filePath = join(dataDirectory, 'uploads', file.fileName);
                    const embedding = await ai.createVideoEmbedding({
                        filePath,
                    });

                    await prisma.file.update({
                        where: { id: file.id },
                        data: { embedding },
                    });

                    try {
                        await remove(fileSearchDb, file.id);
                    } catch {
                        //
                    }

                    await insert(fileSearchDb, {
                        id: file.id,
                        fileName: file.fileName,
                        mimeType: file.mimeType,
                        embedding,
                        textEmbedding: (file.textEmbedding as number[]) || undefined,
                    });

                    sendToUser(file.authorId, 'file:update', { id: file.id, embedding });
                }

                if (job.type === AIJobType.GenerateOcrText) {
                    if (!job.fileId) throw new Error('Missing fileId');

                    const file = await prisma.file.findUnique({
                        where: { id: job.fileId },
                        select: {
                            id: true,
                            fileName: true,
                            mimeType: true,
                            ocrText: true,
                            textEmbedding: true,
                        },
                    });

                    if (!file) throw new Error('File not found');

                    const filePath = join(dataDirectory, 'uploads', file.fileName);
                    const text = await ai.performOcrOnImage(filePath);

                    await prisma.file.update({
                        where: { id: file.id },
                        data: { ocrText: text },
                    });

                    if (text && !file.textEmbedding) {
                        await enqueueAIJob({
                            userId: job.userId,
                            fileId: file.id,
                            type: AIJobType.GenerateTextEmbedding,
                        });
                    }
                }

                if (job.type === AIJobType.GenerateTextEmbedding) {
                    if (!job.fileId) throw new Error('Missing fileId');

                    const file = await prisma.file.findUnique({
                        where: { id: job.fileId },
                        select: {
                            id: true,
                            fileName: true,
                            mimeType: true,
                            ocrText: true,
                            embedding: true,
                            textEmbedding: true,
                            authorId: true,
                        },
                    });

                    if (!file) throw new Error('File not found');

                    if (file.textEmbedding) {
                        await completeAIJob(job.id);
                        return { id: job.id, result: 'skip' };
                    }

                    await enqueueAIJob({
                        userId: job.userId,
                        fileId: file.id,
                        type: AIJobType.DetectPII,
                    });

                    const filePath = join(dataDirectory, 'uploads', file.fileName);
                    const baseText =
                        file.ocrText ??
                        (ai.TEXT_EMBEDDING_SUPPORTED_EXTENSIONS.includes(extname(file.fileName))
                            ? await readUtf8Snippet(filePath, 120_000)
                            : null);

                    if (!baseText) {
                        await completeAIJob(job.id);
                        return { id: job.id, result: 'skip' };
                    }

                    const embedding = await ai.createSemanticTextEmbedding(baseText);
                    if (!embedding) {
                        await completeAIJob(job.id);
                        return { id: job.id, result: 'skip' };
                    }

                    await prisma.file.update({
                        where: { id: file.id },
                        data: { textEmbedding: embedding },
                    });

                    try {
                        await remove(fileSearchDb, file.id);
                    } catch {
                        //
                    }

                    await insert(fileSearchDb, {
                        id: file.id,
                        fileName: file.fileName,
                        mimeType: file.mimeType,
                        embedding: (file.embedding as number[]) || undefined,
                        textEmbedding: embedding,
                    });

                    sendToUser(file.authorId, 'file:update', {
                        id: file.id,
                        textEmbedding: embedding,
                    });
                }

                if (job.type === AIJobType.GenerateImageCaption) {
                    if (!job.fileId) throw new Error('Missing fileId');

                    const file = await prisma.file.findUnique({
                        where: { id: job.fileId },
                        select: {
                            id: true,
                            fileName: true,
                            authorId: true,
                            embedding: true,
                            textEmbedding: true,
                        },
                    });

                    if (!file) throw new Error('File not found');

                    const filePath = join(dataDirectory, 'uploads', file.fileName);
                    const caption = await ai.generateImageCaption(filePath);

                    await prisma.file.update({
                        where: { id: file.id },
                        data: { caption },
                    });

                    try {
                        await remove(fileSearchDb, file.id);
                    } catch {
                        //
                    }

                    await insert(fileSearchDb, {
                        id: file.id,
                        fileName: file.fileName,
                        embedding: (file.embedding as number[]) || undefined,
                        textEmbedding: (file.textEmbedding as number[]) || undefined,
                        caption,
                    });

                    sendToUser(file.authorId, 'file:update', { id: file.id, caption });
                }

                if (job.type === AIJobType.DetectPII) {
                    if (!job.fileId) throw new Error('Missing fileId');

                    const file = await prisma.file.findUnique({
                        where: { id: job.fileId },
                        select: {
                            id: true,
                            fileName: true,
                            authorId: true,
                            ocrText: true,
                        },
                    });
                    if (!file) throw new Error('File not found');

                    const filePath = join(dataDirectory, 'uploads', file.fileName);
                    const { piiDetected, reasons } = await ai.detectPII(filePath);

                    if (piiDetected) {
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
