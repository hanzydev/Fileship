import { AIJobStatus, type AIJobType } from '~~/generated/prisma/enums';

const AI_JOB_LEASE_MS = Math.max(60_000, +(process.env.AI_JOB_LEASE_MS ?? 15 * 60_000));
const AI_JOB_DEQUEUE_RETRIES = Math.max(1, +(process.env.AI_JOB_DEQUEUE_RETRIES ?? 5));

export const AI_ENABLED = (process.env.NUXT_PUBLIC_AI_ENABLED || 'true') === 'true';

export interface EnqueueAIJobOptions {
    userId: string;
    fileId?: string | null;
    type: AIJobType;
}

export interface DequeueAIJobOptions {
    workerId: string;
}

export const enqueueAIJob = async (options: EnqueueAIJobOptions) => {
    if (!AI_ENABLED) return;

    if (options.fileId) {
        const existing = await prisma.aIJob.findFirst({
            where: {
                userId: options.userId,
                fileId: options.fileId,
                type: options.type,
                status: { in: [AIJobStatus.Pending, AIJobStatus.Processing] },
            },
            select: { id: true },
        });
        if (existing) return existing;
    }

    return prisma.aIJob.create({
        data: {
            userId: options.userId,
            fileId: options.fileId ?? null,
            type: options.type,
            status: AIJobStatus.Pending,
        },
        select: { id: true },
    });
};

export const dequeueAIJob = async ({ workerId }: DequeueAIJobOptions) => {
    if (!AI_ENABLED) return;

    for (let i = 0; i < AI_JOB_DEQUEUE_RETRIES; i++) {
        const claimed = await prisma.$transaction(async (tx) => {
            const now = new Date();
            const leaseCutoff = new Date(now.getTime() - AI_JOB_LEASE_MS);

            const pending = await tx.aIJob.findFirst({
                where: { status: AIJobStatus.Pending, lockedAt: null },
                orderBy: { createdAt: 'asc' },
                select: { id: true },
            });

            const candidateId = pending?.id;
            if (candidateId) {
                const res = await tx.aIJob.updateMany({
                    where: { id: candidateId, status: AIJobStatus.Pending, lockedAt: null },
                    data: {
                        status: AIJobStatus.Processing,
                        lockedAt: now,
                        lockedBy: workerId,
                        attempts: { increment: 1 },
                    },
                });

                if (res.count !== 1) return null;
                return tx.aIJob.findUnique({ where: { id: candidateId } });
            }

            const stale = await tx.aIJob.findFirst({
                where: { status: AIJobStatus.Processing, lockedAt: { lt: leaseCutoff } },
                orderBy: { lockedAt: 'asc' },
                select: { id: true },
            });

            const staleId = stale?.id;
            if (!staleId) return null;

            const res = await tx.aIJob.updateMany({
                where: {
                    id: staleId,
                    status: AIJobStatus.Processing,
                    lockedAt: { lt: leaseCutoff },
                },
                data: {
                    status: AIJobStatus.Processing,
                    lockedAt: now,
                    lockedBy: workerId,
                    attempts: { increment: 1 },
                },
            });

            if (res.count !== 1) return null;
            return tx.aIJob.findUnique({ where: { id: staleId } });
        });

        if (claimed) return claimed;
    }

    return null;
};

export const completeAIJob = async (jobId: string) => {
    if (!AI_ENABLED) return;

    return prisma.aIJob.update({
        where: { id: jobId },
        data: { status: AIJobStatus.Completed, lockedAt: null, lockedBy: null, error: null },
    });
};

export const failAIJob = async (jobId: string, error: string) => {
    if (!AI_ENABLED) return;

    return prisma.aIJob.update({
        where: { id: jobId },
        data: { status: AIJobStatus.Failed, lockedAt: null, lockedBy: null, error },
    });
};
