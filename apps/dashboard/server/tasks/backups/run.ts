export default defineTask({
    meta: {
        name: 'backups:run',
        description: 'Creates the scheduled backups that are due and rotates old automatic backups',
    },
    async run() {
        if (!appState.ready || !(await getSettings()).scheduledBackupsEnabled) return { result: 0 };

        return { result: await runDueBackupSchedules() };
    },
});
