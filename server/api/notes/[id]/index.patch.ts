import { z } from 'zod';

import { sendToUser } from '~~/server/plugins/socketIO';

const validationSchema = z
    .object(
        {
            title: z
                .string({
                    invalid_type_error: 'Invalid title',
                })
                .min(1, 'Title must be at least 1 character')
                .max(32, 'Title must be at most 32 characters')
                .optional(),
            content: z
                .string({
                    invalid_type_error: 'Invalid content',
                })
                .min(1, 'Content must be at least 1 character')
                .max(20000, 'Content must be at most 20000 characters')
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

    const noteId = getRouterParam(event, 'id');

    const findNoteById = await prisma.note.findUnique({
        where: {
            id: noteId,
        },
    });

    if (!findNoteById) {
        throw createError({
            statusCode: 404,
            statusMessage: 'Not Found',
            message: 'Note not found',
        });
    }

    if (findNoteById.authorId !== currentUser.id) {
        throw createError({
            statusCode: 403,
            statusMessage: 'Forbidden',
            message: 'You do not have permission to perform this action',
        });
    }

    const updatedNote = await prisma.note.update({
        where: {
            id: noteId,
        },
        data: body.data,
    });

    await createLog(event, {
        action: 'Update Note',
        message: `Updated note ${findNoteById.title}`,
    });

    sendToUser(currentUser.id, 'update:note', updatedNote);

    return updatedNote;
});
