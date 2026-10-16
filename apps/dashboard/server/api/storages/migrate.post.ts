import { z } from 'zod';

const validationSchema = z
    .object({
        verificationData: z.any().optional(),
    })
    .optional();

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

    await verifySession(event, body.data?.verificationData);

    if (storageMigration.progress || storageImport.progress) {
        throw createError({
            status: 400,
            message: storageMigration.progress
                ? 'A migration is already running'
                : 'Wait for the import to finish',
        });
    }

    runTask('storage:migrate').catch(() => null);
});
