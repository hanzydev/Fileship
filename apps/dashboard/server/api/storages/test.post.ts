import { z } from 'zod';

const validationSchema = storageInputSchema.and(z.object({ id: z.string().optional() }));

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

    await testStorage(
        body.data.type,
        await withStoredSecrets(body.data.type, body.data.config, storage),
    );
});
