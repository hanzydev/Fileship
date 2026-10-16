import dayjs from 'dayjs';

const fillHistory = (reports: { day: Date; payload: unknown }[]) => {
    const history: { day: string; users: number; files: number }[] = [];
    const last = reports.at(-1);
    let index = 0;
    let current = { users: 0, files: 0 };

    for (let day = reports[0]?.day; day && last && day <= last.day; day = addDays(day, 1)) {
        if (reports[index]?.day.getTime() === day.getTime()) {
            const usage = (reports[index]!.payload as StoredReport).usage;
            current = { users: usage?.users?.total ?? 0, files: usage?.files?.total ?? 0 };
            index++;
        }

        history.push({ day: toDateString(day), ...current });
    }

    return history;
};

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
        history: fillHistory(instance.reports),
    };
});
