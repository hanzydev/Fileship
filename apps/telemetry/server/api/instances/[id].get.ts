import dayjs from 'dayjs';

export default defineEventHandler(async (event) => {
    adminOnly(event);

    const instance = await prisma.instance.findUnique({
        where: { id: getRouterParam(event, 'id') },
        include: {
            reports: {
                where: { day: { gte: dayjs().subtract(1, 'year').toDate() } },
                orderBy: { day: 'asc' },
                select: { day: true, payload: true },
            },
        },
    });

    if (!instance) {
        throw createError({
            status: 404,
            message: 'Instance not found',
        });
    }

    return {
        ...serializeInstance(instance),
        report: instance.report as StoredReport,
        retentionDays: useRuntimeConfig().instanceRetentionDays,
        history: instance.reports.map(({ day, payload }) => {
            const report = payload as StoredReport;

            return {
                day: toDateString(day),
                version: report.version ?? '',
                users: report.usage?.users?.total ?? 0,
                files: report.usage?.files?.total ?? 0,
                storage: report.usage?.files?.bytes ?? 0,
            };
        }),
    };
});
