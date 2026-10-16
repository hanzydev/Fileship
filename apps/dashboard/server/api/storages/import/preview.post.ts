import { z } from 'zod';

const validationSchema = z.object({
    storageId: z.string().nullable(),
});

export default defineEventHandler(async (event) => {
    superAdminOnly(event);

    const body = await readValidatedBody(event, validationSchema.safeParse);
    if (!body.success) throw invalidBodyError(body.error);

    const { storageId } = body.data;

    if (storageId && !(await prisma.storage.findUnique({ where: { id: storageId } }))) {
        throw apiError(404, 'errorsSystem.storageNotFound');
    }

    const importPath = storageId ? null : await localImportPath();

    const files = await findImportableFiles(storageId).catch((error) => {
        throw apiError(400, 'errorsSystem.storageListFailed', { error: error.message });
    });

    return {
        count: files.length,
        size: files.reduce((total, file) => total + file.size, 0),
        importPath,
    };
});
