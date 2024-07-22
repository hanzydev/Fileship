import defu from 'defu';
import { z } from 'zod';

import { defaultEmbed } from '~~/utils/constants';
import type { IEmbed } from '~~/utils/types';

const validationSchema = z
    .object({
        title: z.string({ invalid_type_error: 'Invalid title' }).optional(),
        description: z
            .string({ invalid_type_error: 'Invalid description' })
            .optional(),
        siteName: z
            .string({ invalid_type_error: 'Invalid site name' })
            .optional(),
        color: z
            .string({ invalid_type_error: 'Invalid color' })
            .startsWith('#', 'Color must start with #')
            .length(7, 'Color must be 7 characters')
            .optional(),
        enabled: z.boolean({
            invalid_type_error: 'Invalid enabled',
        }),
    })
    .strict('Body contains unexpected keys');

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;
    if (!currentUser) {
        throw createError({
            statusCode: 401,
            statusMessage: 'Unauthorized',
            message: 'You do not have permission to perform this action',
        });
    }

    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw createError({
            statusCode: 400,
            statusMessage: 'Bad Request',
            message: 'Invalid body',
            data: body.error.format(),
        });
    }

    const embed = defu(body.data, currentUser.embed, defaultEmbed);

    await prisma.user.update({
        where: {
            id: currentUser.id,
        },
        data: {
            embed,
        },
    });

    return embed as IEmbed;
});
