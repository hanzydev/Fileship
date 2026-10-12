export default defineEventHandler(async (event) => {
    userOnly(event);

    const backup = await findBackup(event.context.user!.id, getRouterParam(event, 'id')!);
    const stream = await (await getStorage(backup.storageId)).read(backupKey(backup));

    setResponseHeaders(event, {
        'Content-Type': 'application/gzip',
        'Content-Length': backup.size.toString(),
        'Content-Disposition': `attachment; filename="${backup.id}.tgz"`,
    });

    return sendStream(event, stream);
});
