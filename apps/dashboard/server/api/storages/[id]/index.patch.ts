import { z } from 'zod';

const validationSchema = z.object({
    name: z.string().trim().min(1, 'errorsSystem.storageNameRequired').max(64),
    config: z.record(z.string(), z.any()),
    verificationData: z.any().optional(),
});

export default defineEventHandler(async (event) => {
    superAdminOnly(event);

    const storage = await prisma.storage.findUnique({
        where: { id: getRouterParam(event, 'id') },
    });

    if (!storage) {
        throw apiError(404, 'errorsSystem.storageNotFound');
    }

    const body = await readValidatedBody(event, validationSchema.safeParse);
    const config = storageConfigSchemas[storage.type].safeParse(body.data?.config);

    if (!body.success || !config.success) {
        throw apiError(400, 'errorsCommon.invalidBody', undefined, {
            formErrors: {
                ...translateIssues(body.error)?.format(),
                ...(config.error && { config: translateIssues(config.error).format() }),
            },
        });
    }

    await verifySession(event, body.data.verificationData);

    const mergedConfig = await withStoredSecrets(storage.type, config.data, storage);

    await testStorage(storage.type, mergedConfig);

    const updated = await prisma.storage.update({
        where: { id: storage.id },
        data: {
            name: body.data.name,
            config: await encrypt(JSON.stringify(mergedConfig)),
        },
    });

    await closeStorage(storage.id);

    await createLog(event, {
        action: 'Update Storage',
        message: `Updated storage ${updated.name}`,
    });

    return serializeStorage(updated);
});
