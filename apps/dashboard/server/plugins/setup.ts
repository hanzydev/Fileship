export default defineNitroPlugin(async () => {
    await getSettings();

    await runTask('db:createDefaultUser');
    await runTask('db:resetBackupRestoreState');

    await runTask('dir:clearTemp');
    await runTask('dir:createDirectories');

    runTask('media:sync');

    await runTask('search:addToOrama');

    await runTask('ai:syncJobs');

    appState.ready = true;
});
