export default defineEventHandler(async (event) => {
    userOnly(event);

    const notes = await prisma.note.findMany({
        where: {
            authorId: event.context.user!.id,
        },
        orderBy: {
            createdAt: 'desc',
        },
    });

    return notes.map((note) => ({
        ...note,
        publicUrl: note.public
            ? buildPublicUrl(event, event.context.user!.domains, `/note/${note.id}`)
            : undefined,
    }));
});
