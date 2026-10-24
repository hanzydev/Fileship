import { z } from 'zod';

import { EmailTokenType } from '#shared/prisma/enums';

const validationSchema = z.object({
    token: z.string().min(1).max(128),
});

export default defineEventHandler(async (event) => {
    const query = await getValidatedQuery(event, validationSchema.safeParse);
    if (!query.success) throw apiError(400, 'errorsEmail.invalidLink');

    await rateLimit(`passwordResetLink:${getClientIp(event)}`, 20, 15 * 60 * 1000);

    const emailToken = await findEmailToken(query.data.token, EmailTokenType.ResetPassword);

    if (!emailToken || emailToken.user.email !== emailToken.email) {
        throw apiError(400, 'errorsEmail.invalidLink');
    }

    return {
        username: emailToken.user.username,
        totpRequired: emailToken.user.totpEnabled,
    };
});
