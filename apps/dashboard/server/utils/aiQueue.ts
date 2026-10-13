import { AIJobStatus, type AIJobType } from '#shared/prisma/enums';
import type { AIJob } from '~~/generated/prisma/client';

export const AI_MAX_ATTEMPTS = 3;

export const enqueueAIJob = async (options: {
    userId: string;
    fileId: string;
    type: AIJobType;
}) => {
    if (!(await isAiEnabled())) return null;

    return prisma.aIJob.create({ data: options, select: { id: true } }).catch((error) => {
        if (error?.code === 'P2002') return null;
        throw error;
    });
};

export const claimAIJob = async (workerId: string) => {
    const { aiJobLeaseMs } = await getSettings();
    const now = new Date();
    const leaseCutoff = new Date(now.getTime() - aiJobLeaseMs);

    const [job] = await prisma.$queryRaw<AIJob[]>`
        UPDATE "AIJob"
        SET "status" = 'Processing',
            "lockedAt" = ${now.toISOString()}::timestamp,
            "lockedBy" = ${workerId},
            "attempts" = "attempts" + 1,
            "updatedAt" = ${now.toISOString()}::timestamp
        WHERE "id" = (
            SELECT "id" FROM "AIJob"
            WHERE "status" = 'Pending'
               OR ("status" = 'Processing' AND ("lockedAt" IS NULL OR "lockedAt" < ${leaseCutoff.toISOString()}::timestamp))
            ORDER BY "createdAt" ASC
            LIMIT 1
            FOR UPDATE SKIP LOCKED
        )
        RETURNING *`;

    return job ?? null;
};

export const renewAIJob = (jobId: string, workerId: string) =>
    prisma.aIJob.updateMany({
        where: { id: jobId, lockedBy: workerId, status: AIJobStatus.Processing },
        data: { lockedAt: new Date() },
    });

export const completeAIJob = (jobId: string) => prisma.aIJob.deleteMany({ where: { id: jobId } });

export const failAIJob = (jobId: string, error: string) =>
    prisma.aIJob.updateMany({
        where: { id: jobId },
        data: { status: AIJobStatus.Failed, lockedAt: null, lockedBy: null, error },
    });

export const retryAIJobLater = (jobId: string, error: string) =>
    prisma.aIJob.updateMany({
        where: { id: jobId },
        data: { lockedAt: new Date(), lockedBy: null, error },
    });
