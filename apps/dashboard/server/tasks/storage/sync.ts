import consola from 'consola';

type StoredFile = { id: string; fileName: string; authorId: string; storageId: string | null };

export default defineTask({
    meta: {
        name: 'storage:sync',
        description: 'Removes files whose stored data is missing',
    },
    async run() {
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
            const name = storageId ? `storage ${storageId}` : 'local storage';
            const stored = await getStorage(storageId)
                .then((storage) => storage.list('uploads/'))
                .catch(() => null);

            if (!stored) {
                consola.warn(`Could not list ${name}, skipping its cleanup.`);
                continue;
            }

            const storageMissing = files.filter((file) => !stored.has(file.fileName));

            if (!stored.size && storageMissing.length) {
                consola.warn(
                    `The ${name} is empty but the database has files for it. Skipping its cleanup, check its configuration.`,
                );
                continue;
            }

            missing.push(...storageMissing);
        }

        if (!missing.length) return { result: 0 };

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

        return { result: missing.length };
    },
});
