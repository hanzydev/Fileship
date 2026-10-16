import { z } from 'zod';

const validationSchema = z.object({
    storageId: z.string().nullable(),
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

    const { storageId } = body.data;

    if (storageId && !(await prisma.storage.findUnique({ where: { id: storageId } }))) {
        throw createError({
            status: 404,
            message: 'Storage not found',
        });
    }

    const importPath = storageId ? null : await localImportPath();

    const files = await findImportableFiles(storageId).catch((error) => {
        throw createError({
            status: 400,
            message: `Could not list the storage: ${error.message}`,
        });
    });

    return {
        count: files.length,
        size: files.reduce((total, file) => total + file.size, 0),
        importPath,
    };
});
