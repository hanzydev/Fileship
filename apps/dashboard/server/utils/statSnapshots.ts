export const getStatSnapshots = async () => {
    const dayAgo = new Date(Date.now() - 24 * 60 * 60 * 1000);

    const [stat, prevStat] = await Promise.all([
        prisma.stat.findFirst({ orderBy: { createdAt: 'desc' } }),
        prisma.stat.findFirst({
            where: { createdAt: { lte: dayAgo } },
            orderBy: { createdAt: 'desc' },
        }),
    ]);

    return {
        dayAgo,
        stat,
        prevStat: prevStat || (await prisma.stat.findFirst({ orderBy: { createdAt: 'asc' } })),
    };
};
