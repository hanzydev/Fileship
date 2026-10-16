const STALE_RESTORE_AGE = 2 * 60 * 60 * 1000;

export default defineTask({
    meta: {
        name: 'db:resetBackupRestoreState',
        description: "Resets users' stale backup restore state",
    },
    async run() {
        await prisma.user.updateMany({
            where: {
                backupRestoreState: { not: null },
                OR: [
                    { backupRestoreStartedAt: null },
                    { backupRestoreStartedAt: { lt: new Date(Date.now() - STALE_RESTORE_AGE) } },
                ],
            },
            data: {
                backupRestoreState: null,
                backupRestoreStartedAt: null,
            },
        });

        return { result: 'success' };
    },
});
