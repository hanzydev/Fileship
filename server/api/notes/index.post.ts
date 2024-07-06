import { z } from 'zod';

import { sendByFilter, sendToUser } from '~~/server/plugins/socketIO';
import { canTakeNotes, isAdmin } from '~~/utils/user';

const validationSchema = z.object(
    {
        title: z
            .string({
                invalid_type_error: 'Invalid title',
                required_error: 'Missing title',
            })
            .min(1, 'Title must be at least 1 character')
            .max(32, 'Title must be at most 32 characters'),
        content: z
            .string({
                invalid_type_error: 'Invalid content',
                required_error: 'Missing content',
            })
            .min(1, 'Content must be at least 1 character')
            .max(20000, 'Content must be at most 20000 characters'),
    },
    { invalid_type_error: 'Invalid body', required_error: 'Missing body' },
);

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user!;
    if (!canTakeNotes(currentUser)) {
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

    const note = await prisma.note.create({
        data: {
            title: body.data.title,
            content: body.data.content,
            authorId: currentUser.id,
        },
    });

    const log = await prisma.log.create({
        data: {
            action: 'Take Note',
            userId: currentUser.id,
            message: 'Took a note',
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

    sendToUser(currentUser.id, 'create:note', note);
    await sendByFilter(
        (socket) => isAdmin(socket.handshake.auth.user)!,
        'create:log',
        log,
    );

    return note;
});
