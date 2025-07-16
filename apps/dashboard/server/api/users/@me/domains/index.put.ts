import { z } from 'zod';

const validationSchema = z
    .array(z.string())
    .transform((value) => value.filter((domain) => domain.length).map((domain) => domain.trim()));

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw createError({
            statusCode: 400,
            statusMessage: 'Bad Request',
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
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
