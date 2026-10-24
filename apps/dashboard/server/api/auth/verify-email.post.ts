import { z } from 'zod';

import { EmailTokenType } from '#shared/prisma/enums';

const validationSchema = z.object({
    token: z.string().min(1).max(128),
});

export default defineEventHandler(async (event) => {
    const body = await readValidatedBody(event, validationSchema.safeParse);
    if (!body.success) throw invalidBodyError(body.error);

    await rateLimit(`verifyEmail:${getClientIp(event)}`, 10, 15 * 60 * 1000);

    const emailToken = await findEmailToken(body.data.token, EmailTokenType.VerifyEmail);

    if (!emailToken || emailToken.user.pendingEmail !== emailToken.email) {
        throw apiError(400, 'errorsEmail.invalidLink');
    }

    if (
        await prisma.user.count({
            where: { email: emailToken.email, id: { not: emailToken.userId } },
        })
    ) {
        throw apiError(409, 'errorsEmail.emailTaken');
    }

    if (!(await consumeEmailToken(emailToken.id))) throw apiError(400, 'errorsEmail.invalidLink');

    await prisma.emailToken.deleteMany({ where: { userId: emailToken.userId } });

    const user = await updateUserEmail(emailToken.userId, {
        email: emailToken.email,
        pendingEmail: null,
    });

    await createLog(Object.assign({}, event, { context: { user: emailToken.user } }), {
        action: 'Verify Email',
        message: `Confirmed the email address ${emailToken.email}`,
    });

    return { email: user.email };
});
