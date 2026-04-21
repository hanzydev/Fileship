export default defineTask({
    meta: {
        name: 'db:resetBackupRestoreState',
        description: "Resets users' backup restore state",
    },
    async run() {
        await prisma.user.updateMany({
            data: {
                backupRestoreState: null,
            },
        });

        return { result: 'success' };
    },
});
