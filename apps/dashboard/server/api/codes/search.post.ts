import { z } from 'zod';

import { search } from '@orama/orama';

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

    const userCodes = await prisma.code.findMany({
        where: {
            authorId: event.context.user!.id,
        },
        select: {
            id: true,
        },
    });

    const searched = await search(codeSearchDb, { term: body.data.query });
    return searched.hits
        .filter((hit) => userCodes.some((code) => code.id === hit.id))
        .map((hit) => hit.id);
});
