import consola from 'consola';
import dayjs from 'dayjs';
import { extname } from 'pathe';

import { AIJobType } from '#shared/prisma/enums';
import type { AIJob, Prisma } from '~~/generated/prisma/client';
import { DbNull } from '~~/generated/prisma/internal/prismaNamespace';

export const visitorFileWhere = async (owner: {
    aiSettings: unknown;
}): Promise<Prisma.FileWhereInput> => {
    if (!(await isAiEnabled()) || (owner.aiSettings as any)?.enabled === false)
        return { piiDetected: false };

    return {
        piiDetected: false,
        NOT: {
            aiAnalyzedAt: null,
            OR: [...AI_IMAGE_EXTENSIONS, ...AI_TEXT_EXTENSIONS].map((extension) => ({
                fileName: { endsWith: extension, mode: 'insensitive' as const },
            })),
        },
    };
};

export const AI_JOB_TYPES = {
    image: AIJobType.AnalyzeImage,
    video: AIJobType.AnalyzeVideo,
    text: AIJobType.AnalyzeText,
} as const;

export const enqueueFileAnalysis = async (file: {
    id: string;
    authorId: string;
    fileName: string;
}) => {
    const kind = getAIFileKind(file.fileName);
    if (!kind) return null;

    return enqueueAIJob({ userId: file.authorId, fileId: file.id, type: AI_JOB_TYPES[kind] });
};

const readFileText = async (path: string) => {
    const text = await readUtf8Snippet(path, 200_000);
    const extension = extname(path).toLowerCase();

    if (['.html', '.htm', '.xml'].includes(extension))
        return text
            .replace(/<(script|style)[\s\S]*?<\/\1>/gi, ' ')
            .replace(/<[^>]+>/g, ' ')
            .replace(/&[a-z#0-9]+;/gi, ' ');

    if (extension === '.rtf') return text.replace(/\\[a-z]+-?\d* ?|[{}]/gi, ' ');

    return text;
};

const analysisSelect = {
    id: true,
    authorId: true,
    fileName: true,
    storageId: true,
    metadata: true,
    ocrText: true,
    caption: true,
    embeddingModel: true,
    textEmbeddingModel: true,
    piiOverride: true,
    aiAnalyzedAt: true,
} satisfies Prisma.FileSelect;

type AnalysisFile = Prisma.FileGetPayload<{ select: typeof analysisSelect }>;

const saveFile = async (file: AnalysisFile, data: Prisma.FileUpdateInput) => {
    await prisma.file.update({ where: { id: file.id }, data });
    Object.assign(file, data);
};

const detectFilePii = async (file: AnalysisFile, text: string) => {
    if (file.piiOverride !== null) return;

    const { piiDetected, reasons } = await ai.detectPII(text, file.caption ?? '');

    await saveFile(file, { piiDetected, piiReasons: reasons });
    sendToUser(file.authorId, 'file:update', { id: file.id, piiDetected, piiReasons: reasons });
};

const finishAnalysis = async (file: AnalysisFile) => {
    const aiAnalyzedAt = new Date();

    await saveFile(file, { aiAnalyzedAt });
    await upsertFileSearchEntry(file.id);
    sendToUser(file.authorId, 'file:update', { id: file.id, aiAnalyzedAt });
};

const analyzeImage = async (file: AnalysisFile, path: string) => {
    const preset = await getAIPreset();
    const image = await ai.loadImage(path);
    let textChanged = false;

    if (file.embeddingModel !== preset.image.key) {
        await saveFile(file, {
            embedding: await ai.embedImage(image),
            embeddingModel: preset.image.key,
        });
    }

    if (file.ocrText === null) {
        await saveFile(file, { ocrText: await ai.recognizeText(path) });
        textChanged = true;
    }

    if (file.caption === null) {
        const caption = await ai.captionImage(image);

        await saveFile(file, { caption });
        sendToUser(file.authorId, 'file:update', { id: file.id, caption });
        textChanged = true;
    }

    const text = [file.caption, file.ocrText].filter(Boolean).join('\n');

    if (textChanged || file.textEmbeddingModel !== preset.text.key) {
        const textEmbedding = await ai.embedText(text);

        await saveFile(file, {
            textEmbedding: textEmbedding ?? DbNull,
            textEmbeddingModel: preset.text.key,
        });
    }

    if (textChanged || !file.aiAnalyzedAt) await detectFilePii(file, file.ocrText ?? '');
};

const analyzeVideo = async (file: AnalysisFile, path: string) => {
    const preset = await getAIPreset();

    if (file.embeddingModel === preset.image.key) return;

    await saveFile(file, {
        embedding: await ai.embedVideo(path, (file.metadata as any)?.duration),
        embeddingModel: preset.image.key,
    });
};

const analyzeText = async (file: AnalysisFile, path: string) => {
    const preset = await getAIPreset();
    const text = await readFileText(path);

    if (!file.aiAnalyzedAt || file.textEmbeddingModel !== preset.text.key) {
        const textEmbedding = await ai.embedText(text);

        await saveFile(file, {
            textEmbedding: textEmbedding ?? DbNull,
            textEmbeddingModel: preset.text.key,
        });
    }

    if (!file.aiAnalyzedAt) await detectFilePii(file, text);
};

const processAIJob = async (job: AIJob) => {
    const user = await prisma.user.findUnique({
        where: { id: job.userId },
        select: { aiSettings: true },
    });

    if (!((user?.aiSettings as any)?.enabled ?? true)) return 'disabled';

    const file = await prisma.file.findUnique({
        where: { id: job.fileId, deletedAt: null },
        select: analysisSelect,
    });

    if (!file) return 'missing';

    await withLocalFile(file.storageId, uploadKey(file.fileName), async (path) => {
        if (job.type === AIJobType.AnalyzeImage) await analyzeImage(file, path);
        if (job.type === AIJobType.AnalyzeVideo) await analyzeVideo(file, path);
        if (job.type === AIJobType.AnalyzeText) await analyzeText(file, path);

        if (job.type === AIJobType.DetectPII) {
            await detectFilePii(
                file,
                getAIFileKind(file.fileName) === 'text'
                    ? await readFileText(path)
                    : (file.ocrText ?? ''),
            );
            return;
        }

        await finishAnalysis(file);
    });

    return 'done';
};

export const runAIJob = async (job: AIJob, workerId: string) => {
    const start = Date.now();
    const { aiJobLeaseMs } = await getSettings();
    const heartbeat = setInterval(
        () => renewAIJob(job.id, workerId).catch(() => null),
        Math.max(10_000, aiJobLeaseMs / 3),
    );

    try {
        if (job.attempts > AI_MAX_ATTEMPTS) {
            await failAIJob(job.id, job.error ?? 'The job stopped responding too many times');
            return;
        }

        const result = await processAIJob(job);
        await completeAIJob(job.id);

        consola.success(
            `${dayjs().format('YYYY-MM-DD HH:mm:ss')} - AI job ${job.type} for ${job.fileId} ${result} in ${Date.now() - start}ms`,
        );
    } catch (error: any) {
        const message = error?.message || 'Unknown error';

        if (job.attempts >= AI_MAX_ATTEMPTS) await failAIJob(job.id, message);
        else await retryAIJobLater(job.id, message);

        consola.error(
            `${dayjs().format('YYYY-MM-DD HH:mm:ss')} - AI job ${job.type} for ${job.fileId} failed (attempt ${job.attempts}): ${message}`,
        );
    } finally {
        clearInterval(heartbeat);
    }
};
