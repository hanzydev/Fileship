import { AIJobStatus } from '#shared/prisma/enums';

export default defineEventHandler(async (event) => {
    superAdminOnly(event);

    const { count } = await prisma.aIJob.deleteMany({ where: { status: AIJobStatus.Failed } });

    if (count && (await isAiEnabled())) await runTask('ai:syncJobs');

    await createLog(event, {
        action: 'Retry AI Jobs',
        message: `Retried ${count} failed AI jobs`,
    });

    return { retried: count };
});
