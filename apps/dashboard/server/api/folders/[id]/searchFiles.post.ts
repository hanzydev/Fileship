import { z } from 'zod';

const validationSchema = z.object({
    query: z
        .string()
        .min(1, 'Search query must be at least 1 character long')
        .max(100, 'Search query must be at most 100 characters long'),
    mode: z.enum(['vector', 'fulltext']),
});

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;
    const folderId = getRouterParam(event, 'id');

    const findFolderById = await prisma.folder.findUnique({
        where: {
            id: folderId,
        },
        include: {
            files: {
                orderBy: {
                    createdAt: 'desc',
                },
                select: {
                    id: true,
                    password: true,
                },
            },
        },
    });

    if (!findFolderById) {
        throw createError({
            statusCode: 404,
            message: 'Folder not found',
        });
    }

    if (findFolderById.authorId !== currentUser?.id) {
        if (!findFolderById.public) {
            throw forbiddenError;
        } else {
            findFolderById.files = findFolderById.files.filter((f) => !f.password);
        }
    }

    const body = await readValidatedBody(event, validationSchema.safeParse);
    if (!body.success) {
        throw createError({
            statusCode: 400,
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    const ids = await searchFileIds(body.data.query, body.data.mode);

    const allowedIds = new Set(findFolderById.files.map((f) => f.id));
    return ids.filter((id) => allowedIds.has(id));
});
