import { z } from 'zod';

const validationSchema = storageInputSchema.and(
    z.object({ id: z.string().optional(), verificationData: z.any().optional() }),
);

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

    const storage = body.data.id
        ? await prisma.storage.findUnique({ where: { id: body.data.id } })
        : null;

    const config = body.data.config as Record<string, any>;

    if (storage && storageSecretKeys[body.data.type].some((key) => !config[key])) {
        await verifySession(event, body.data.verificationData);
    }

    await testStorage(body.data.type, await withStoredSecrets(body.data.type, config, storage));
});
