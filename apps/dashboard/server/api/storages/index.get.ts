export default defineEventHandler(async (event) => {
    superAdminOnly(event);

    const [storages, files, backups] = await prisma.$transaction([
        prisma.storage.findMany({
            orderBy: { createdAt: 'asc' },
            include: { _count: { select: { files: true, backups: true } } },
        }),
        prisma.file.count({ where: { storageId: null } }),
        prisma.backup.count({ where: { storageId: null } }),
    ]);

    return {
        activeStorageId: await getActiveStorageId(),
        migration: storageMigration.progress,
        import: storageImport.progress,
        local: { files, backups },
        items: await Promise.all(
            storages.map(async ({ _count, ...storage }) => ({
                ...(await serializeStorage(storage)),
                ..._count,
            })),
        ),
    };
});
