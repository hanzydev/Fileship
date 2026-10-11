import { z } from 'zod';

import { update } from '@orama/orama';

const validationSchema = z
    .object({
        name: z
            .string()
            .min(1, 'Name must be at least 1 character')
            .max(32, 'Name must be at most 32 characters')
            .optional(),
        files: z.array(z.string()).optional(),
        public: z.boolean().optional(),
    })
    .strict();

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

    const folderId = getRouterParam(event, 'id');

    const findFolderById = await prisma.folder.findUnique({
        where: {
            id: folderId,
            authorId: currentUser.id,
        },
        include: {
            files: {
                select: {
                    id: true,
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

    let connect: { id: string }[] = [];
    let disconnect: { id: string }[] = [];

    if (body.data.files) {
        const files = await prisma.file.findMany({
            where: {
                id: {
                    in: body.data.files,
                },
                authorId: currentUser.id,
            },
            select: {
                id: true,
            },
        });

        if (files.length !== body.data.files.length) {
            throw createError({
                statusCode: 400,
                message: 'Invalid files',
            });
        }

        connect = body.data.files
            .filter((fileId) => !findFolderById.files.some((f) => f.id === fileId))
            .map((fileId) => ({ id: fileId }));

        disconnect = findFolderById.files
            .filter((f) => !body.data.files?.includes(f.id))
            .map((f) => ({ id: f.id }));
    }

    const _updatedFolder = await prisma.folder.update({
        where: {
            id: folderId,
        },
        include: folderInclude,
        data: {
            name: body.data.name,
            public: body.data.public,
            files: {
                connect,
                disconnect,
            },
        },
    });

    const updatedFolder = serializeFolder(event, _updatedFolder);

    await update(folderSearchDb, _updatedFolder.id, {
        id: updatedFolder.id,
        authorId: updatedFolder.authorId,
        name: updatedFolder.name,
    });

    await createLog(event, {
        action: 'Update Folder',
        message: `Updated folder ${findFolderById.name}`,
    });

    sendToUser(currentUser.id, 'folder:update', updatedFolder);

    return updatedFolder;
});
