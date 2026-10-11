import { z } from 'zod';

import { insert } from '@orama/orama';

const validationSchema = z.object({
    name: z
        .string()
        .min(1, 'Name must be at least 1 character')
        .max(32, 'Name must be at most 32 characters'),
    files: z.array(z.string()).optional(),
    public: z.boolean().optional().default(false),
});

export default defineEventHandler(async (event) => {
    fileUploaderOnly(event);

    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw createError({
            statusCode: 400,
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    if (body.data.files?.length) {
        const files = await prisma.file.findMany({
            where: {
                id: {
                    in: body.data.files,
                },
                authorId: currentUser.id,
            },
        });

        if (files.length !== body.data.files.length) {
            throw createError({
                statusCode: 400,
                message: 'Invalid files',
            });
        }
    }

    const _folder = await prisma.folder.create({
        data: {
            name: body.data.name,
            public: body.data.public,
            files: {
                connect: body.data.files?.map((file) => ({ id: file })),
            },
            authorId: currentUser.id,
        },
        include: folderInclude,
    });

    const folder = serializeFolder(event, _folder);

    await insert(folderSearchDb, {
        id: folder.id,
        authorId: folder.authorId,
        name: folder.name,
    });

    await createLog(event, {
        action: 'Create Folder',
        message: `Created folder ${folder.name}`,
    });

    sendToUser(currentUser.id, 'folder:create', folder);

    return folder;
});
