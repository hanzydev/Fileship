import { z } from 'zod';

const validationSchema = z.object({
    storageId: z.string().nullable(),
    userId: z.string(),
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

    if (storageImport.progress || storageMigration.progress) {
        throw createError({
            status: 400,
            message: storageImport.progress
                ? 'An import is already running'
                : 'Wait for the storage migration to finish',
        });
    }

    const { storageId, userId } = body.data;

    if (storageId && !(await prisma.storage.findUnique({ where: { id: storageId } }))) {
        throw createError({
            status: 404,
            message: 'Storage not found',
        });
    }

    const owner = await prisma.user.findUnique({ where: { id: userId } });

    if (!owner) {
        throw createError({
            status: 404,
            message: 'User not found',
        });
    }

    event.waitUntil(runStorageImport(event, storageId, owner));
});
