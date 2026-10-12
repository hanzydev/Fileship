import consola from 'consola';

type StoredFile = { id: string; fileName: string; authorId: string; storageId: string | null };

const storageName = (storageId: string | null) =>
    storageId ? `storage ${storageId}` : 'local storage';

const listStorage = (storageId: string | null, prefix: string) =>
    getStorage(storageId)
        .then((storage) => storage.list(prefix))
        .catch(() => {
            consola.warn(`Could not list ${storageName(storageId)}, skipping its cleanup.`);
            return null;
        });

const warnEmpty = (storageId: string | null) =>
    consola.warn(
        `The ${storageName(storageId)} is empty but the database has data for it. Skipping its cleanup, check its configuration.`,
    );

const syncFiles = async () => {
    const groups = new Map<string | null, StoredFile[]>();
    let cursor: string | undefined;

    while (true) {
        const files = await prisma.file.findMany({
            select: { id: true, fileName: true, authorId: true, storageId: true },
            orderBy: { id: 'asc' },
            take: 500,
            ...(cursor && { skip: 1, cursor: { id: cursor } }),
        });

        if (!files.length) break;

        for (const file of files) {
            groups.set(file.storageId, [...(groups.get(file.storageId) ?? []), file]);
        }

        cursor = files.at(-1)!.id;
    }

    const missing: StoredFile[] = [];

    for (const [storageId, files] of groups) {
        const stored = await listStorage(storageId, 'uploads/');
        if (!stored) continue;

        const storageMissing = files.filter((file) => !stored.has(file.fileName));

        if (!stored.size && storageMissing.length) {
            warnEmpty(storageId);
            continue;
        }

        missing.push(...storageMissing);
    }

    if (!missing.length) return 0;

    await prisma.file.deleteMany({ where: { id: { in: missing.map((file) => file.id) } } });
    await removeFileData(missing);

    for (const file of missing) sendToUser(file.authorId, 'file:delete', file.id);

    const names = missing.map((file) => file.fileName).join(', ');

    await createLog(null, {
        action: 'Delete File',
        message: `Removed ${missing.length} files missing from storage: ${names}`,
        system: true,
    });

    consola.info(`Removed ${missing.length} files missing from storage.`);

    return missing.length;
};

const syncBackups = async () => {
    const storages = await prisma.storage.findMany({ select: { id: true } });
    const users = new Set(
        (await prisma.user.findMany({ select: { id: true } })).map(({ id }) => id),
    );

    let changed = 0;

    for (const storageId of [null, ...storages.map(({ id }) => id)]) {
        const stored = await listStorage(storageId, 'backups/');
        if (!stored) continue;

        const backups = await prisma.backup.findMany({ where: { storageId } });
        const known = new Set(backups.map((backup) => `${backup.userId}/${backup.id}.tgz`));
        const missing = backups.filter(
            (backup) => !stored.has(`${backup.userId}/${backup.id}.tgz`),
        );

        if (!stored.size && missing.length) {
            warnEmpty(storageId);
            continue;
        }

        const unindexed = [...stored].flatMap(([key, object]) => {
            const [, userId, id] = key.match(/^([\w-]+)\/([\w-]+)\.tgz$/) ?? [];

            return userId && users.has(userId) && !known.has(key)
                ? [{ id: id!, userId, size: object.size, storageId, createdAt: object.modifiedAt }]
                : [];
        });

        await prisma.backup.deleteMany({ where: { id: { in: missing.map(({ id }) => id) } } });
        await prisma.backup.createMany({ data: unindexed, skipDuplicates: true });

        for (const backup of missing) sendToUser(backup.userId, 'backup:delete', backup.id);

        changed += missing.length + unindexed.length;
    }

    return changed;
};

export default defineTask({
    meta: {
        name: 'storage:sync',
        description: 'Removes files and backups whose stored data is missing',
    },
    async run() {
        return { result: (await syncFiles()) + (await syncBackups()) };
    },
});
