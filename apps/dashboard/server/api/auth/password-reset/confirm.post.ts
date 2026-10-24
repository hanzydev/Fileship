import { hash } from 'argon2';
import { z } from 'zod';

import { EmailTokenType } from '#shared/prisma/enums';

const validationSchema = z.object({
    token: z.string().min(1).max(128),
    password: z.string().min(8, 'errorsAuth.passwordMin').max(48, 'errorsAuth.passwordMax'),
    totp: z
        .string()
        .length(6, 'errorsAuth.totpLength')
        .regex(/^\d+$/, 'errorsAuth.totpNumber')
        .optional(),
});

export default defineEventHandler(async (event) => {
    const body = await readValidatedBody(event, validationSchema.safeParse);
    if (!body.success) throw invalidBodyError(body.error);

    await rateLimit(`passwordResetLink:${getClientIp(event)}`, 20, 15 * 60 * 1000);

    const emailToken = await findEmailToken(body.data.token, EmailTokenType.ResetPassword);

    if (!emailToken || emailToken.user.email !== emailToken.email) {
        throw apiError(400, 'errorsEmail.invalidLink');
    }

    const user = emailToken.user;

    if (user.totpEnabled) {
        if (!body.data.totp) {
            throw apiError(400, 'errorsAuth.totpRequired', undefined, { totpRequired: true });
        }

        await rateLimit(`passwordResetTotp:${emailToken.id}`, 5, 60 * 60 * 1000);

        if (!(await verifyTotpToken(user.id, user.totpSecret!, body.data.totp))) {
            throw apiError(401, 'errorsAuth.invalidTotp');
        }
    }

    if (!(await consumeEmailToken(emailToken.id))) throw apiError(400, 'errorsEmail.invalidLink');

    const currentSessionId =
        event.context.user?.id === user.id ? event.context.user.currentSessionId : undefined;

    await prisma.$transaction([
        prisma.user.update({
            where: { id: user.id },
            data: { password: await hash(body.data.password) },
        }),
        prisma.emailToken.deleteMany({
            where: { userId: user.id, type: EmailTokenType.ResetPassword },
        }),
        prisma.session.deleteMany({
            where: { userId: user.id, id: { not: currentSessionId } },
        }),
    ]);

    await logoutSockets(
        (socketUser) =>
            socketUser.id === user.id && socketUser.currentSessionId !== currentSessionId,
    );

    sendToUser(user.id, 'session:deleteAll', null);

    await createLog(Object.assign({}, event, { context: { user } }), {
        action: 'Reset Password',
        message: 'Reset their password with an email link',
    });
});
