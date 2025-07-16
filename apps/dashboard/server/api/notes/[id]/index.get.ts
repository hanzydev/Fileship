export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const noteId = getRouterParam(event, 'id');

    const findNoteById = await prisma.note.findUnique({
        where: {
            id: noteId,
            authorId: currentUser.id,
        },
    });

    if (!findNoteById) {
        throw createError({
            statusCode: 404,
            message: 'Note not found',
        });
    }

    return findNoteById;
});
