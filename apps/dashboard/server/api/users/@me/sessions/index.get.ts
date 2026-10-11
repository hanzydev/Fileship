export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const query = await readListQuery(event);

    const where = { userId: currentUser.id };

    const [items, total] = await Promise.all([
        prisma.session.findMany({
            where,
            select: {
                id: true,
                ip: true,
                location: true,
                os: true,
                platform: true,
                lastSeen: true,
            },
            orderBy: [{ lastSeen: 'desc' }, { id: 'asc' }],
            skip: query.offset,
            take: query.limit,
        }),
        prisma.session.count({ where }),
    ]);

    return { items, total };
});
