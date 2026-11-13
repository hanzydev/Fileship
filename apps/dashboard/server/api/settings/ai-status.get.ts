import { AIJobStatus } from '#shared/prisma/enums';

export default defineEventHandler(async (event) => {
    superAdminOnly(event);

    const counts = await prisma.aIJob.groupBy({ by: ['status'], _count: true });
    const count = (status: AIJobStatus) => counts.find((row) => row.status === status)?._count ?? 0;

    return {
        pending: count(AIJobStatus.Pending),
        processing: count(AIJobStatus.Processing),
        failed: count(AIJobStatus.Failed),
        serviceUp: aiService.up,
        modelCacheSize: aiService.modelsBytes,
    };
});
