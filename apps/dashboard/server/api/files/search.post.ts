import { z } from 'zod';

const validationSchema = z.object({
    query: z
        .string()
        .min(1, 'Search query must be at least 1 character long')
        .max(100, 'Search query must be at most 100 characters long'),
    mode: z.enum(['vector', 'fulltext']),
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

    const ids = await searchFileIds(body.data.query, body.data.mode);

    const files = await prisma.file.findMany({
        where: {
            id: { in: ids },
            authorId: event.context.user!.id,
        },
        select: { id: true },
    });

    const allowedIds = new Set(files.map((f) => f.id));
    return ids.filter((id) => allowedIds.has(id));
});
