export default defineNitroPlugin(async () => {
    await runTask('db:createDefaultUser');
    await runTask('db:resetBackupRestoreState');

    await runTask('dir:clearTemp');
    await runTask('dir:createDirectories');

    await runTask('search:generateEmbeddings');
    await runTask('search:addToOrama');
});
