import { z } from 'zod';

const validationSchema = storageInputSchema.and(
    z.object({
        name: z.string().trim().min(1, 'Name is required').max(64),
        verificationData: z.any().optional(),
    }),
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

    await verifySession(event, body.data.verificationData);
    await testStorage(body.data.type, body.data.config);

    const storage = await prisma.storage.create({
        data: {
            name: body.data.name,
            type: body.data.type,
            config: await encrypt(JSON.stringify(body.data.config)),
        },
    });

    await createLog(event, {
        action: 'Create Storage',
        message: `Created storage ${storage.name}`,
    });

    return serializeStorage(storage);
});
