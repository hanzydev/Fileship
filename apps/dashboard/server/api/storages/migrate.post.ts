import { z } from 'zod';

const validationSchema = z
    .object({
        verificationData: z.any().optional(),
    })
    .optional();

export default defineEventHandler(async (event) => {
    superAdminOnly(event);

    const body = await readValidatedBody(event, validationSchema.safeParse);
    if (!body.success) throw invalidBodyError(body.error);

    await verifySession(event, body.data?.verificationData);

    if (storageMigration.progress || storageImport.progress) {
        throw apiError(
            400,
            storageMigration.progress
                ? 'errorsSystem.migrationRunning'
                : 'errorsSystem.waitForImport',
        );
    }

    runTask('storage:migrate').catch(() => null);
});
