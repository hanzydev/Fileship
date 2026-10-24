export default defineEventHandler(async (event) => {
    userOnly(event);
    const currentUser = event.context.user!;

    if (
        (await prisma.backup.count({ where: { userId: currentUser.id } })) >=
        backupLimitOf(currentUser)
    ) {
        throw apiError(400, 'errorsUsers.backupLimitReached');
    }

    event.waitUntil(
        createBackup(currentUser).then((created) =>
            createLog(event, {
                action: 'Create Backup',
                message: `Created backup ${created.id}`,
            }),
        ),
    );
});
