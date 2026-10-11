export default defineEventHandler(async (event) => {
    adminOnly(event);

    const actions = await prisma.log.findMany({
        distinct: ['action'],
        orderBy: { action: 'asc' },
        select: { action: true },
    });

    return actions.map((log) => log.action);
});
