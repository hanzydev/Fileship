export default defineTask({
    meta: {
        name: 'db:resetBackupRestoreState',
        description: "Resets users' backup restore state",
    },
    run() {
        prisma.user.updateMany({
            data: {
                backupRestoreState: null,
            },
        });

        return { result: 'success' };
    },
});
