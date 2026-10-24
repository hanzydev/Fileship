export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;

    return serializeBackupSchedule(
        await prisma.backupSchedule.findUnique({ where: { userId: currentUser.id } }),
        currentUser,
    );
});
