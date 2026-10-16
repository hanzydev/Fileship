import defu from 'defu';

export default defineEventHandler(async (event) => {
    const noteId = getRouterParam(event, 'id');
    const user = event.context.user;

    const findNoteById = await prisma.note.findUnique({
        where: {
            id: noteId,
            deletedAt: null,
        },
        include: {
            author: {
                select: {
                    domains: true,
                    embed: true,
                },
            },
        },
    });

    if (!findNoteById) {
        throw apiError(404, 'errorsContent.noteNotFound');
    }

    if (!findNoteById.public && findNoteById.authorId !== user?.id) {
        throw apiError(403, 'errorsContent.notePageForbidden');
    }

    const embed = defu(findNoteById.author.embed, defaultEmbed) as IEmbed;

    return {
        ...findNoteById,
        author: undefined,
        authorId: findNoteById.authorId === user?.id ? findNoteById.authorId : undefined,
        publicUrl: findNoteById.public
            ? buildPublicUrl(event, findNoteById.author.domains, `/note/${findNoteById.id}`)
            : undefined,
        embed: {
            enabled: embed.enabled,
            color: embed.color,
        },
    };
});
