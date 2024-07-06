import { z } from 'zod';

import { sendByFilter, sendToUser } from '~~/server/plugins/socketIO';
import { isAdmin } from '~~/utils/user';

const validationSchema = z
    .object(
        {
            name: z
                .string({
                    invalid_type_error: 'Invalid name',
                    required_error: 'Missing name',
                })
                .min(1, 'Name must be at least 1 character')
                .max(32, 'Name must be at most 32 characters')
                .optional(),
            files: z
                .array(
                    z.string({ invalid_type_error: 'File must be a string' }),
                    {
                        invalid_type_error: 'Invalid files',
                    },
                )
                .optional(),
            public: z
                .boolean({ invalid_type_error: 'Invalid public' })
                .optional(),
        },
        { invalid_type_error: 'Invalid body', required_error: 'Missing body' },
    )
    .strict({
        message: 'Body contains unexpected keys',
    });

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;
    if (!currentUser) {
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

    const folderId = getRouterParam(event, 'id');

    const findFolderById = await prisma.folder.findUnique({
        where: {
            id: folderId,
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
            statusMessage: 'Not Found',
            message: 'Folder not found',
        });
    }

    if (findFolderById.authorId !== currentUser.id) {
        throw createError({
            statusCode: 403,
            statusMessage: 'Forbidden',
            message: 'You do not have permission to perform this action',
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
            },
            select: {
                id: true,
            },
        });

        if (files.length !== body.data.files.length) {
            throw createError({
                statusCode: 400,
                statusMessage: 'Bad Request',
                message: 'Invalid files',
            });
        }

        connect = body.data.files
            .filter(
                (fileId) => !findFolderById.files.some((f) => f.id === fileId),
            )
            .map((fileId) => ({ id: fileId }));

        disconnect = findFolderById.files
            .filter((f) => !body.data.files?.includes(f.id))
            .map((f) => ({ id: f.id }));
    }

    const updatedFolder = await prisma.folder.update({
        where: {
            id: folderId,
        },
        include: {
            files: {
                select: {
                    id: true,
                },
            },
        },
        data: {
            name: body.data.name,
            public: body.data.public,
            files: {
                connect,
                disconnect,
            },
        },
    });

    const log = await prisma.log.create({
        data: {
            action: 'Update Folder',
            userId: currentUser.id,
            message: `Updated folder ${findFolderById.name}`,
            ip: getRequestIP(event, { xForwardedFor: true })!,
        },
        include: {
            user: {
                select: {
                    id: true,
                    username: true,
                },
            },
        },
    });

    sendToUser(currentUser.id, 'update:folder', updatedFolder);
    await sendByFilter(
        (socket) => isAdmin(socket.handshake.auth.user)!,
        'create:log',
        log,
    );

    connect.forEach((file) => {
        sendToUser(currentUser.id, 'folder:file:add', {
            folderId,
            fileId: file.id,
        });
    });

    disconnect.forEach((file) => {
        sendToUser(currentUser.id, 'folder:file:remove', {
            folderId,
            fileId: file.id,
        });
    });

    return updatedFolder;
});
