import { z } from 'zod';

const validationSchema = z.object({
    oidcIssuerUrl: settingsSchema.shape.oidcIssuerUrl.min(1, 'instance.oidcIssuerUrlInvalid'),
});

export default defineEventHandler(async (event) => {
    superAdminOnly(event);

    const body = await readValidatedBody(event, validationSchema.safeParse);
    if (!body.success) throw invalidBodyError(body.error);

    await rateLimit(`oidcTest:${event.context.user!.id}`, 20, 10 * 60 * 1000);

    try {
        const config = await discoverOidc(body.data.oidcIssuerUrl, 'fileship', '');
        return { issuer: config.serverMetadata().issuer };
    } catch (error) {
        throw apiError(502, 'errorsOidc.discoveryFailed', { error: (error as Error).message });
    }
});
