export default defineNitroPlugin(async () => {
    await getSettings();

    await runTask('db:createDefaultUser');
    await runTask('db:resetBackupRestoreState');

    await runTask('dir:clearTemp');
    await runTask('dir:createDirectories');
    await runTask('storage:sync');

    runTask('media:sync');
    runTask('files:hash');

    await runTask('search:addToOrama');

    await runTask('ai:syncJobs');

    appState.ready = true;

    setTimeout(() => runTask('telemetry:collect'), 5 * 60_000);
});
