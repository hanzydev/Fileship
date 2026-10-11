import { z } from 'zod';

const validationSchema = z.object({
    query: z
        .string()
        .min(1, 'Search query must be at least 1 character long')
        .max(100, 'Search query must be at most 100 characters long'),
});

export default defineEventHandler(async (event) => {
    userOnly(event);

    const body = await readValidatedBody(event, validationSchema.safeParse);
    if (!body.success) {
        throw createError({
            statusCode: 400,
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    const userNotes = await prisma.note.findMany({
        where: {
            authorId: event.context.user!.id,
        },
        select: {
            id: true,
        },
    });

    const ids = await searchDbIds(noteSearchDb, body.data.query);
    return ids.filter((id) => userNotes.some((note) => note.id === id));
});
