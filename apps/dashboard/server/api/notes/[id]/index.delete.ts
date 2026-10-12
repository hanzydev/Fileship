export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const noteId = getRouterParam(event, 'id');

    const findNoteById = await prisma.note.findUnique({
        where: {
            id: noteId,
            authorId: currentUser.id,
            deletedAt: null,
        },
    });

    if (!findNoteById) {
        throw createError({
            status: 404,
            message: 'Note not found',
        });
    }

    const isTrashed = await isTrashEnabled();

    if (isTrashed) {
        await prisma.note.update({ where: { id: noteId }, data: { deletedAt: new Date() } });
    } else {
        await deleteNotesForever([noteId!]);
    }

    await createLog(event, {
        action: 'Delete Note',
        message: isTrashed
            ? `Moved note ${findNoteById.title} to trash`
            : `Deleted note ${findNoteById.title}`,
    });

    sendToUser(currentUser.id, 'note:delete', noteId);
    sendToUser(currentUser.id, 'trash:update', null);
});
