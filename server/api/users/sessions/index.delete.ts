import { z } from 'zod';

import { sendByFilter, sendToUser } from '~~/server/plugins/socketIO';

const validationSchema = z
    .object({
        verificationData: z
            .string({
                invalid_type_error: 'Invalid data',
            })
            .optional(),
    })
    .optional();

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

    await verifySession(currentUser, body.data?.verificationData);

    await prisma.session.deleteMany({
        where: {
            userId: currentUser.id,
            id: {
                not: currentUser.currentSessionId,
            },
        },
    });

    await sendByFilter(
        (socket) =>
            socket.handshake.auth.user.id === currentUser.id &&
            socket.handshake.auth.user.currentSessionId !==
                currentUser.currentSessionId,
        'logout',
        null,
    );
    sendToUser(currentUser.id, 'delete:session:all', null);
});
