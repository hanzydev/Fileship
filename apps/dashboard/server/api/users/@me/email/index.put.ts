import { z } from 'zod';

import { EmailTokenType } from '#shared/prisma/enums';

const validationSchema = z.object({
    email: emailAddressSchema,
    verificationData: z.any().optional(),
});

export default defineEventHandler(async (event) => {
    userOnly(event);

    if (!(await isEmailEnabled())) throw apiError(400, 'errorsEmail.disabled');

    const body = await readValidatedBody(event, validationSchema.safeParse);
    if (!body.success) throw invalidBodyError(body.error);

    await verifySession(event, body.data.verificationData);

    const currentUser = event.context.user!;
    const { email } = body.data;

    if (email === currentUser.email) {
        await prisma.emailToken.deleteMany({
            where: { userId: currentUser.id, type: EmailTokenType.VerifyEmail },
        });

        return updateUserEmail(currentUser.id, { pendingEmail: null });
    }

    if (await prisma.user.count({ where: { email } })) {
        throw apiError(409, 'errorsEmail.emailTaken', undefined, {
            formErrors: { email: { _errors: [translate('errorsEmail.emailTaken')] } },
        });
    }

    await rateLimit(`emailVerification:${currentUser.id}`, 5, 60 * 60 * 1000);

    const token = await createEmailToken(
        currentUser.id,
        EmailTokenType.VerifyEmail,
        email,
        24 * 60 * 60 * 1000,
    );

    try {
        await sendEmail({
            to: email,
            language: emailLanguage(currentUser, event),
            template: 'verifyEmail',
            params: { username: currentUser.username },
            path: `/verify-email?token=${token}`,
        });
    } catch {
        await prisma.emailToken.deleteMany({
            where: { userId: currentUser.id, type: EmailTokenType.VerifyEmail },
        });

        throw apiError(502, 'errorsEmail.sendFailed');
    }

    await createLog(event, {
        action: 'Update Email',
        message: `Sent a confirmation link to ${email}`,
    });

    return updateUserEmail(currentUser.id, { pendingEmail: email });
});
