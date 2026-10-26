import { z } from 'zod';

const validationSchema = z
    .object({
        verificationData: z.any().optional(),
    })
    .optional();

export default defineEventHandler(async (event) => {
    userOnly(event);

    const body = await readValidatedBody(event, validationSchema.safeParse);
    if (!body.success) throw invalidBodyError(body.error);

    const settings = await getSettings();

    if (!isOidcConfigured(settings)) {
        throw apiError(400, 'errorsOidc.unavailable');
    }

    if (!isSiteHost(event, settings)) {
        throw apiError(400, 'errorsOidc.wrongAddress', { url: settings.siteUrl });
    }

    await verifySession(event, body.data?.verificationData);
    await rateLimit(`oidcLink:${event.context.user!.id}`, 10, 15 * 60 * 1000);

    const url = await startOidcFlow(event, settings, {
        intent: 'link',
        userId: event.context.user!.id,
    }).catch(() => null);

    if (!url) throw apiError(502, 'errorsOidc.unavailable');

    return { url };
});
