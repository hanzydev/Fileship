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
            statusMessage: 'Bad Request',
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
            },
        });

        if (files.length !== body.data.files.length) {
            throw createError({
                statusCode: 400,
                statusMessage: 'Bad Request',
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
        include: {
            files: {
                select: {
                    id: true,
                    createdAt: true,
                },
            },
        },
    });

    const folder = {
        ..._folder,
        files: _folder.files
            .sort((a, b) => b.createdAt.getTime() - a.createdAt.getTime())
            .map((file) => file.id),
        publicUrl: _folder.public
            ? buildPublicUrl(event, currentUser.domains, `/folder/${_folder.id}`)
            : undefined,
    };

    await insert(folderSearchDb, {
        id: folder.id,
        name: folder.name,
    });

    await createLog(event, {
        action: 'Create Folder',
        message: `Created folder ${folder.name}`,
    });

    sendToUser(currentUser.id, 'create:folder', folder);

    folder.files.forEach((file) => {
        sendToUser(currentUser.id, 'folder:file:add', {
            folderId: folder.id,
            fileId: file,
        });
    });

    return folder;
});
