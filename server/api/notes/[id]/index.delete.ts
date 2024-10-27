export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const noteId = getRouterParam(event, 'id');

    const findNoteById = await prisma.note.findUnique({
        where: {
            id: noteId,
        },
    });

    if (!findNoteById) {
        throw createError({
            statusCode: 404,
            statusMessage: 'Not Found',
            message: 'Note not found',
        });
    }

    if (findNoteById.authorId !== currentUser.id) throw forbiddenError;

    await prisma.note.delete({
        where: {
            id: noteId,
        },
    });

    await createLog(event, {
        action: 'Delete Note',
        message: `Deleted note ${findNoteById.title}`,
    });

    sendToUser(currentUser.id, 'delete:note', noteId);
});
