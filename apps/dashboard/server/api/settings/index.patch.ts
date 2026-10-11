import { z } from 'zod';

const validationSchema = z.object({
    ...Object.fromEntries(
        Object.entries(settingsSchema.shape).map(([key, schema]) => [
            key,
            (schema as z.ZodType).nullable().optional(),
        ]),
    ),
    verificationData: z.any().optional(),
});

export default defineEventHandler(async (event) => {
    superAdminOnly(event);

    const body = await readValidatedBody(event, validationSchema.safeParse);
    if (!body.success) {
        throw createError({
            status: 400,
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    await verifySession(event, body.data.verificationData);

    const { verificationData: _, ...patch } = body.data;
    const keys = Object.keys(patch) as InstanceSettingKey[];

    if (!keys.length) {
        throw createError({
            status: 400,
            message: 'Nothing to update',
        });
    }

    const wasAiEnabled = await isAiEnabled();

    await updateSettings(patch as never);

    const publicSettings = await getPublicSettings();

    await createLog(event, {
        action: 'Update Settings',
        message: `Updated instance settings: ${keys.join(', ')}`,
    });

    sendToAll('settings:update', publicSettings);

    if (!wasAiEnabled && publicSettings.aiEnabled && appState.ready) {
        runTask('ai:syncJobs').catch(() => null);
    }

    return publicSettings;
});
