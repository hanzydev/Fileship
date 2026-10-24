import { z } from 'zod';

import { EmailTokenType } from '#shared/prisma/enums';

const validationSchema = z.object({
    login: z.string().trim().min(1).max(254),
    turnstile: z.string().optional(),
});

export default defineEventHandler(async (event) => {
    if (!(await isEmailEnabled())) throw apiError(400, 'errorsEmail.disabled');

    const body = await readValidatedBody(event, validationSchema.safeParse);
    if (!body.success) throw invalidBodyError(body.error);

    const login = body.data.login.toLowerCase();

    await rateLimit(`passwordReset:${getClientIp(event)}`, 5, 15 * 60 * 1000);
    await rateLimit(`passwordReset:${login}`, 3, 60 * 60 * 1000);

    if (!(await verifyTurnstile(body.data.turnstile))) {
        throw apiError(400, 'errorsAuth.invalidTurnstile');
    }

    const user = await prisma.user.findFirst({
        where: {
            OR: [{ username: body.data.login }, { email: login }],
            email: { not: null },
        },
    });

    if (!user) return;

    event.waitUntil(
        (async () => {
            const token = await createEmailToken(
                user.id,
                EmailTokenType.ResetPassword,
                user.email!,
                60 * 60 * 1000,
            );

            await sendEmail({
                to: user.email!,
                language: emailLanguage(user, event),
                template: 'resetPassword',
                params: { username: user.username },
                path: `/reset-password?token=${token}`,
            });

            await createLog(Object.assign({}, event, { context: { user } }), {
                action: 'Request Password Reset',
                message: 'Requested a password reset link',
            });
        })().catch(() => null),
    );
});
