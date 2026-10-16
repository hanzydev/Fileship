import consola from 'consola';

const notOn = (storageId: string | null) =>
    storageId
        ? { OR: [{ storageId: null }, { storageId: { not: storageId } }] }
        : { storageId: { not: null } };

const moveObject = async (from: string | null, to: string | null, key: string) => {
    await withLocalFile(from, key, async (path) => (await getStorage(to)).put(key, path));
};

const reportProgress = (progress: { done: number; total: number } | null) => {
    storageMigration.progress = progress;
    sendByFilter((user) => !!user?.superAdmin, 'storage:migrate', progress);
};

export default defineTask({
    meta: {
        name: 'storage:migrate',
        description: 'Moves every file and backup to the active storage',
    },
    async run() {
        const target = await getActiveStorageId();
        const files = await prisma.file.findMany({
            where: notOn(target),
            select: { id: true, fileName: true, storageId: true },
        });
        const backups = await prisma.backup.findMany({ where: notOn(target) });
        const total = files.length + backups.length;

        let done = 0;
        let failed = 0;

        const step = async (key: string, from: string | null, update: () => Promise<unknown>) => {
            try {
                await moveObject(from, target, key);
                await update();
                await (await getStorage(from)).remove(key).catch(() => null);
            } catch (error: any) {
                failed++;
                consola.warn(`Could not move ${key}: ${error.message}`);
            }

            reportProgress({ done: ++done, total });
        };

        reportProgress({ done, total });

        for (const file of files) {
            await step(uploadKey(file.fileName), file.storageId, () =>
                prisma.file.update({ where: { id: file.id }, data: { storageId: target } }),
            );
        }

        for (const backup of backups) {
            await step(backupKey(backup), backup.storageId, () =>
                prisma.backup.update({ where: { id: backup.id }, data: { storageId: target } }),
            );
        }

        reportProgress(null);

        await createLog(null, {
            action: 'Update Settings',
            message: `Moved ${total - failed} of ${total} files and backups to the active storage`,
            system: true,
        });

        return { result: total - failed };
    },
});
