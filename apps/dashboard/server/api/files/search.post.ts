import { z } from 'zod';

import { search } from '@orama/orama';

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
            statusMessage: 'Bad Request',
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    const clip = await getClipInstance();

    const options = { mode: body.data.mode } as any;

    if (body.data.mode === 'vector') {
        options.similarity = 0.2;
        options.vector = {
            value: await clip.createTextEmbedding(body.data.query),
            property: 'embedding',
        };
    } else {
        options.term = body.data.query;
    }

    const userFiles = await prisma.file.findMany({
        where: {
            authorId: event.context.user!.id,
        },
        select: {
            id: true,
        },
    });

    const searched = await search(fileSearchDb, options);
    return searched.hits
        .filter((hit) => userFiles.some((file) => file.id === hit.id))
        .map((hit) => hit.id);
});
