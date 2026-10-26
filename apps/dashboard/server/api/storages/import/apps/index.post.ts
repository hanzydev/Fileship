import { z } from 'zod';

const validationSchema = z.object({
    sourceId: z.string().regex(/^[\w-]{21}$/),
    storageId: z.string().nullable(),
    users: z.record(z.string(), z.string().nullable()),
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

    const { sourceId, storageId, users } = body.data;

    if (storageId && !(await prisma.storage.findUnique({ where: { id: storageId } }))) {
        throw apiError(404, 'errorsSystem.storageNotFound');
    }

    const source = await loadImportSource(sourceId);
    const owners = await resolveImportOwners(source, users);

    event.waitUntil(runAppImport(event, sourceId, source, storageId, owners));
});
