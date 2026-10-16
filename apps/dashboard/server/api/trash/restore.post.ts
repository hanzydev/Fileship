export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, trashSelectionSchema.safeParse);

    if (!body.success) {
        throw invalidBodyError(body.error);
    }

    const trashed = { authorId: currentUser.id, deletedAt: { not: null } };

    const [folders, files] = await Promise.all([
        prisma.folder.findMany({
            where: { ...trashed, id: { in: body.data.folders } },
            select: { id: true, deletedAt: true },
        }),
        prisma.file.findMany({
            where: { ...trashed, id: { in: body.data.files } },
            select: { id: true, folderId: true, folder: { select: { deletedAt: true } } },
        }),
    ]);

    const folderIds = folders.map((folder) => folder.id);
    const isOrphan = (file: (typeof files)[number]) =>
        !!file.folder?.deletedAt && !folderIds.includes(file.folderId!);

    const [, restoredFiles, orphanFiles, restoredNotes, ...folderFiles] = await prisma.$transaction(
        [
            prisma.folder.updateMany({
                where: { id: { in: folderIds } },
                data: { deletedAt: null },
            }),
            prisma.file.updateMany({
                where: {
                    id: { in: files.filter((file) => !isOrphan(file)).map((file) => file.id) },
                },
                data: { deletedAt: null },
            }),
            prisma.file.updateMany({
                where: { id: { in: files.filter(isOrphan).map((file) => file.id) } },
                data: { deletedAt: null, folderId: null },
            }),
            prisma.note.updateMany({
                where: { ...trashed, id: { in: body.data.notes } },
                data: { deletedAt: null },
            }),
            ...folders.map((folder) =>
                prisma.file.updateMany({
                    where: filesTrashedWith(folder),
                    data: { deletedAt: null },
                }),
            ),
        ],
    );

    const counts = {
        files: [restoredFiles, orphanFiles, ...folderFiles].reduce(
            (total, result) => total + result!.count,
            0,
        ),
        folders: folderIds.length,
        notes: restoredNotes!.count,
    };

    await createLog(event, {
        action: 'Restore From Trash',
        message: `Restored ${counts.files} files, ${counts.folders} folders and ${counts.notes} notes`,
    });

    sendToUser(currentUser.id, 'trash:update', null);
    sendToUser(currentUser.id, 'trash:restore', null);

    if (counts.files && (await isAiEnabled()))
        runTask('ai:syncJobs', { payload: { userId: currentUser.id } }).catch(() => null);

    return counts;
});
