import { z } from 'zod';

const validationSchema = z.object({
    code: z.string(),
    password: z.string().max(48),
});

export default defineEventHandler(async (event) => {
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw createError({
            status: 400,
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    await rateLimit(
        `linkUnlock:${getRequestIP(event, { xForwardedFor: true })}`,
        10,
        10 * 60 * 1000,
    );

    const link = await findActiveLink(body.data.code);

    if (link.password !== body.data.password) {
        throw createError({
            status: 401,
            message: 'Invalid password',
        });
    }

    await countLinkClick(event, link);

    return { url: link.url };
});
