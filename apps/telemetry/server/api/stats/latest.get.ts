const growth = (current: number, previous?: number | null) =>
    previous ? Math.round(((current - previous) / previous) * 1000) / 10 : null;

export default defineEventHandler(async (event) => {
    adminOnly(event);

    const latest = await prisma.dailyStat.findFirst({ orderBy: { day: 'desc' } });
    if (!latest) return null;

    const previous = await prisma.dailyStat.findUnique({
        where: { day: addDays(latest.day, -30) },
    });

    const storage = Number(latest.storage);

    return {
        instances: latest.instances,
        users: latest.users,
        files: latest.files,
        storage,
        versions: latest.versions as Record<string, number>,
        data: latest.data as DayData,
        growth: {
            instances: growth(latest.instances, previous?.instances),
            users: growth(latest.users, previous?.users),
            files: growth(latest.files, previous?.files),
            storage: growth(storage, previous && Number(previous.storage)),
        },
    };
});
