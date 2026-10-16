import { z } from 'zod';

const validationSchema = z.object({
    storageId: z.string().nullable(),
    userId: z.string(),
    verificationData: z.any().optional(),
});

export default defineEventHandler(async (event) => {
    superAdminOnly(event);

    const body = await readValidatedBody(event, validationSchema.safeParse);
    if (!body.success) throw invalidBodyError(body.error);

    await verifySession(event, body.data.verificationData);

    if (storageImport.progress || storageMigration.progress) {
        throw apiError(
            400,
            storageImport.progress ? 'errorsSystem.importRunning' : 'errorsSystem.waitForMigration',
        );
    }

    const { storageId, userId } = body.data;

    if (storageId && !(await prisma.storage.findUnique({ where: { id: storageId } }))) {
        throw apiError(404, 'errorsSystem.storageNotFound');
    }

    const owner = await prisma.user.findUnique({ where: { id: userId } });

    if (!owner) {
        throw apiError(404, 'errorsSystem.userNotFound');
    }

    event.waitUntil(runStorageImport(event, storageId, owner));
});
