import { z } from 'zod';

const validationSchema = z.object({
    ...Object.fromEntries(
        Object.entries(settingsSchema.omit({ activeStorageId: true }).shape).map(
            ([key, schema]) => [key, (schema as z.ZodType).nullable().optional()],
        ),
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

    const previous = await getSettings();

    await updateSettings(patch as never);

    const publicSettings = await getPublicSettings();

    await createLog(event, {
        action: 'Update Settings',
        message: `Updated instance settings: ${keys.join(', ')}`,
    });

    sendToAll('settings:update', publicSettings);

    const settings = await getSettings();

    if (
        settings.aiPreset !== previous.aiPreset ||
        settings.aiInferenceThreads !== previous.aiInferenceThreads
    )
        await unloadAIModels();

    if (settings.aiPreset !== previous.aiPreset && appState.ready) await reloadFileVectors();

    if (
        settings.aiEnabled &&
        (!previous.aiEnabled || settings.aiPreset !== previous.aiPreset) &&
        appState.ready
    ) {
        runTask('ai:syncJobs').catch(() => null);
        ai.warmUp().catch(() => null);
    }

    return publicSettings;
});
