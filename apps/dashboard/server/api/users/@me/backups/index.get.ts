export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const query = await readListQuery(event);

    const [backups, total] = await prisma.$transaction([
        prisma.backup.findMany({
            where: { userId: currentUser.id },
            orderBy: { createdAt: 'desc' },
            skip: query.offset,
            take: query.limit,
        }),
        prisma.backup.count({ where: { userId: currentUser.id } }),
    ]);

    return {
        items: backups.map(serializeBackup),
        total,
    };
});
