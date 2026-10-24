import z from 'zod';

const validationSchema = z
    .object({
        permanent: z.boolean().optional(),
    })
    .default({});

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const fileId = getRouterParam(event, 'id');
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw invalidBodyError(body.error);
    }

    const findFileById = await prisma.file.findUnique({
        where: {
            id: fileId,
            deletedAt: null,
        },
        include: { folder: { select: { public: true } } },
    });

    const access = findFileById && (await fileAccessOf(findFileById, currentUser.id));

    if (!findFileById || !access) {
        throw apiError(404, 'errorsFiles.fileNotFound');
    }

    if (!hasFolderAccess(access, 'editor')) throw forbiddenError();

    const isTrashed = !(access === 'owner' && body.data.permanent) && (await isTrashEnabled());

    if (isTrashed) {
        await prisma.file.update({ where: { id: fileId }, data: { deletedAt: new Date() } });
    } else {
        await deleteFilesForever([findFileById]);
    }

    await createLog(event, {
        action: 'Delete File',
        message: isTrashed
            ? `Moved file ${findFileById.fileName} to trash`
            : `Deleted file ${findFileById.fileName}`,
    });

    emitFilesDeleted(
        event,
        findFileById.authorId,
        [findFileById],
        isTrashed ? 'trashed' : 'deleted',
    );

    await sendToFolder(findFileById.authorId, [findFileById.folderId], 'file:delete', fileId);
    sendToUser(findFileById.authorId, 'trash:update', null);
});
