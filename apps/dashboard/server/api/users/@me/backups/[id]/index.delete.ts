export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const backup = await findBackup(currentUser.id, getRouterParam(event, 'id')!);

    await (await getStorage(backup.storageId)).remove(backupKey(backup));
    await prisma.backup.delete({ where: { id: backup.id } });

    await createLog(event, {
        action: 'Delete Backup',
        message: `Deleted backup ${backup.id}`,
    });

    sendToUser(currentUser.id, 'backup:delete', backup.id);
});
