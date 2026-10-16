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

    const storage = await prisma.storage.findUnique({
        where: { id: getRouterParam(event, 'id') },
        include: { _count: { select: { files: true, backups: true } } },
    });

    if (!storage) {
        throw apiError(404, 'errorsSystem.storageNotFound');
    }

    if (
        storage.id === (await getActiveStorageId()) ||
        storage._count.files ||
        storage._count.backups
    ) {
        throw apiError(400, 'errorsSystem.storageInUse');
    }

    await verifySession(event, body.data?.verificationData);

    await prisma.storage.delete({ where: { id: storage.id } });
    await closeStorage(storage.id);

    await createLog(event, {
        action: 'Delete Storage',
        message: `Deleted storage ${storage.name}`,
    });
});
