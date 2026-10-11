import { z } from 'zod';

const validationSchema = z.object({
    files: z.array(z.uuid()).min(1).max(5000),
    folderId: z.string().nullable(),
});

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw createError({
            statusCode: 400,
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    const findFolderById = body.data.folderId
        ? await prisma.folder.findUnique({
              where: {
                  id: body.data.folderId,
                  authorId: currentUser.id,
              },
              select: { id: true, name: true },
          })
        : null;

    if (body.data.folderId && !findFolderById) {
        throw createError({
            statusCode: 404,
            message: 'Folder not found',
        });
    }

    const files = await prisma.file.findMany({
        where: {
            id: { in: body.data.files },
            authorId: currentUser.id,
        },
        select: { id: true },
    });

    const { count } = await prisma.file.updateMany({
        where: {
            id: { in: files.map((file) => file.id) },
        },
        data: {
            folderId: body.data.folderId,
        },
    });

    sendToUser(currentUser.id, 'file:bulkMove', {
        files: files.map((file) => file.id),
        folder: findFolderById,
    });

    await createLog(event, {
        action: 'Bulk Move Files',
        message: findFolderById
            ? `Moved ${count} files to folder ${findFolderById.name}`
            : `Removed ${count} files from their folder`,
    });

    return { count };
});
