import { z } from 'zod';

const validationSchema = z.object({
    token: z.string().min(1).max(128),
});

export default defineEventHandler(async (event) => {
    const query = await getValidatedQuery(event, validationSchema.safeParse);
    if (!query.success) throw apiError(400, 'errorsEmail.invalidInvite');

    await rateLimit(`inviteLookup:${getClientIp(event)}`, 20, 15 * 60 * 1000);

    const invite = await findInvite(query.data.token);
    if (!invite) throw apiError(400, 'errorsEmail.invalidInvite');

    return { email: invite.email, invitedBy: invite.invitedBy.username };
});
