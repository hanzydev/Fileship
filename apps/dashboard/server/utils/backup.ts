type StoredBackup = { id: string; userId: string; storageId: string | null };

export const backupKey = (backup: { id: string; userId: string }) =>
    `backups/${backup.userId}/${backup.id}.tgz`;

export const serializeBackup = (backup: { id: string; size: bigint; createdAt: Date }) => ({
    id: backup.id,
    createdAt: backup.createdAt,
    size: {
        raw: backup.size.toString(),
        formatted: formatSize(backup.size),
    },
});

export const findBackup = async (userId: string, id: string) => {
    const backup = await prisma.backup.findFirst({ where: { id, userId } });

    if (!backup) {
        throw apiError(404, 'errorsSystem.backupNotFound');
    }

    return backup;
};

export const removeBackupData = (backups: StoredBackup[]) =>
    Promise.allSettled(
        backups.map(async (backup) =>
            (await getStorage(backup.storageId)).remove(backupKey(backup)),
        ),
    );
