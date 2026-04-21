import defu from 'defu';

export default defineEventHandler(async (event) => {
    const noteId = getRouterParam(event, 'id');
    const user = event.context.user;

    const findNoteById = await prisma.note.findUnique({
        where: {
            id: noteId,
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
        throw createError({
            statusCode: 404,
            message: 'Note not found',
        });
    }

    if (!findNoteById.public && findNoteById.authorId !== user?.id) {
        throw createError({
            statusCode: 403,
            message: 'You do not have permission to access this page',
        });
    }

    const embed = defu(findNoteById.author.embed, defaultEmbed) as IEmbed;

    return {
        ...findNoteById,
        author: undefined,
        publicUrl: findNoteById.public
            ? buildPublicUrl(event, findNoteById.author.domains, `/note/${findNoteById.id}`)
            : undefined,
        embed: {
            enabled: embed.enabled,
            color: embed.color,
        },
    };
});
