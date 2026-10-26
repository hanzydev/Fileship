import { z } from 'zod';

const validationSchema = z.object({
    sourceId: z.string().regex(/^[\w-]{21}$/),
    storageId: z.string().nullable(),
});

export default defineEventHandler(async (event) => {
    superAdminOnly(event);

    const body = await readValidatedBody(event, validationSchema.safeParse);
    if (!body.success) throw invalidBodyError(body.error);

    const { sourceId, storageId } = body.data;

    if (storageId && !(await prisma.storage.findUnique({ where: { id: storageId } }))) {
        throw apiError(404, 'errorsSystem.storageNotFound');
    }

    return previewAppImport(sourceId, storageId).catch((error) => {
        if (error.statusCode) throw error;
        throw apiError(400, 'errorsSystem.storageListFailed', { error: error.message });
    });
});
