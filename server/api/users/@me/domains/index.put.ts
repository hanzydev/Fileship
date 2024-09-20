import { z } from 'zod';

import { sendToUser } from '~~/server/plugins/socketIO';

const validationSchema = z
    .array(z.string(), {
        invalid_type_error: 'Invalid domains',
        required_error: 'Missing domains',
    })
    .transform((value) =>
        value.filter((domain) => domain.length).map((domain) => domain.trim()),
    );

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

    await prisma.user.update({
        where: {
            id: currentUser.id,
        },
        data: {
            domains: body.data,
        },
    });

    sendToUser(currentUser.id, 'update:domains', body.data);

    return body.data;
});
