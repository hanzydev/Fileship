import { z } from 'zod';

import { sendToUser } from '~~/server/plugins/socketIO';
import { canUploadFiles } from '~~/utils/user';

const validationSchema = z.object(
    {
        name: z
            .string({
                invalid_type_error: 'Invalid name',
                required_error: 'Missing name',
            })
            .min(1, 'Name must be at least 1 character')
            .max(32, 'Name must be at most 32 characters'),
        files: z
            .array(z.string({ invalid_type_error: 'File must be a string' }), {
                invalid_type_error: 'Invalid files',
            })
            .optional(),
        public: z
            .boolean({ invalid_type_error: 'Invalid public' })
            .optional()
            .default(false),
    },
    { invalid_type_error: 'Invalid body', required_error: 'Missing body' },
);

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user!;
    if (!canUploadFiles(currentUser)) {
        throw createError({
            statusCode: 401,
            statusMessage: 'Unauthorized',
            message: 'You do not have permission to perform this action',
        });
    }

    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw createError({
            statusCode: 400,
            statusMessage: 'Bad Request',
            message: 'Invalid body',
            data: body.error.format(),
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

    const folder = await prisma.folder.create({
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
                },
            },
        },
    });

    await createLog(event, {
        action: 'Create Folder',
        message: 'Created a folder',
    });

    sendToUser(currentUser.id, 'create:folder', folder);

    folder.files.forEach((file) => {
        sendToUser(currentUser.id, 'folder:file:add', {
            folderId: folder.id,
            fileId: file.id,
        });
    });

    return folder;
});
