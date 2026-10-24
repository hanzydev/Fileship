import z from 'zod';

const validationSchema = z.object({
    files: z.array(z.uuid()).min(1).max(5000),
    permanent: z.boolean().optional(),
});

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) throw invalidBodyError(body.error);

    const candidates = await prisma.file.findMany({
        where: {
            id: { in: body.data.files },
            deletedAt: null,
        },
        select: {
            id: true,
            fileName: true,
            storageId: true,
            authorId: true,
            folderId: true,
        },
    });

    const folderAccess = new Map<string, Promise<FolderAccess | null>>();
    const canDelete = async (file: (typeof candidates)[number]) => {
        if (file.authorId === currentUser.id) return true;
        if (!file.folderId) return false;

        if (!folderAccess.has(file.folderId))
            folderAccess.set(file.folderId, fileAccessOf(file, currentUser.id));

        return hasFolderAccess(await folderAccess.get(file.folderId)!, 'editor');
    };

    const allowed = await Promise.all(candidates.map(canDelete));
    const filesToBeDeleted = candidates.filter((_, index) => allowed[index]);

    if (!filesToBeDeleted.length) {
        throw apiError(404, 'errorsFiles.noFilesToDelete');
    }

    const fileIds = filesToBeDeleted.map((file) => file.id);

    const isTrashed =
        !(
            body.data.permanent &&
            filesToBeDeleted.every((file) => file.authorId === currentUser.id)
        ) && (await isTrashEnabled());

    if (isTrashed) {
        await prisma.file.updateMany({
            where: { id: { in: fileIds } },
            data: { deletedAt: new Date() },
        });
    } else {
        await deleteFilesForever(filesToBeDeleted);
    }

    for (const ownerId of new Set(filesToBeDeleted.map((file) => file.authorId))) {
        const owned = filesToBeDeleted.filter((file) => file.authorId === ownerId);

        await sendToFolder(
            ownerId,
            [...new Set(owned.map((file) => file.folderId))],
            'file:bulkDelete',
            owned.map((file) => file.id),
        );
        sendToUser(ownerId, 'trash:update', null);
    }

    await createLog(event, {
        action: 'Bulk Delete Files',
        message: isTrashed
            ? `Moved ${fileIds.length} files to trash`
            : `Deleted ${fileIds.length} files`,
    });
});
