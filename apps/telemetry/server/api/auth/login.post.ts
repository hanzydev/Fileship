import { z } from 'zod';

const loginSchema = z.object({
    password: z.string().min(1, 'Password is required').max(256),
});

export default defineEventHandler(async (event) => {
    if (!useRuntimeConfig().adminPassword) {
        throw createError({
            status: 503,
            message: 'Set NUXT_ADMIN_PASSWORD to use the panel',
        });
    }

    const body = await readValidatedBody(event, loginSchema.safeParse);

    if (!body.success) {
        throw createError({
            status: 400,
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    const valid = await isAdminPassword(body.data.password);
    const allowed = await limitFailedAttempts(`login:${getClientIp(event)}`, valid);

    await limitFailedAttempts('login', valid, 50);

    if (!allowed) {
        throw createError({
            status: 401,
            message: 'Wrong password',
        });
    }

    await startSession(event);

    return { admin: true };
});
