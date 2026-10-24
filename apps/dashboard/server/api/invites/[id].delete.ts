export default defineEventHandler(async (event) => {
    adminOnly(event);

    const invite = await prisma.invite.findUnique({
        where: { id: getRouterParam(event, 'id') },
    });

    if (!invite) throw apiError(404, 'errorsEmail.inviteNotFound');

    await prisma.invite.delete({ where: { id: invite.id } });

    await createLog(event, {
        action: 'Delete Invite',
        message: `Withdrew the invite for ${invite.email}`,
    });

    await sendByFilter(isAdmin, 'invite:update', null);
});
