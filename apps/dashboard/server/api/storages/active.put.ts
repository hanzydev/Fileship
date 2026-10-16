import { z } from 'zod';

const validationSchema = z.object({
    id: z.string().nullable(),
    verificationData: z.any().optional(),
});

export default defineEventHandler(async (event) => {
    superAdminOnly(event);

    const body = await readValidatedBody(event, validationSchema.safeParse);
    if (!body.success) throw invalidBodyError(body.error);

    await verifySession(event, body.data.verificationData);

    const storage = body.data.id
        ? await prisma.storage.findUnique({ where: { id: body.data.id } })
        : { name: 'local storage' };

    if (!storage) {
        throw apiError(404, 'errorsSystem.storageNotFound');
    }

    await updateSettings({ activeStorageId: body.data.id });

    sendToAll('settings:update', await getPublicSettings());

    await createLog(event, {
        action: 'Update Settings',
        message: `New files are now stored on ${storage.name}`,
    });
});
