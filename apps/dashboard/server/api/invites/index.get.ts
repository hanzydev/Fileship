export default defineEventHandler(async (event) => {
    adminOnly(event);

    return prisma.invite.findMany({
        where: { expiresAt: { gt: new Date() } },
        orderBy: { createdAt: 'desc' },
        select: inviteSelect,
    });
});
