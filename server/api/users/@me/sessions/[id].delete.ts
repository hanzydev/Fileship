import { z } from 'zod';

const validationSchema = z
    .object({
        verificationData: z
            .string({
                invalid_type_error: 'Invalid verification data',
            })
            .optional(),
    })
    .optional();

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
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

    const sessionId = getRouterParam(event, 'id');
    const findSessionById = await prisma.session.findUnique({
        where: {
            id: sessionId,
        },
    });

    if (!findSessionById) {
        throw createError({
            statusCode: 404,
            statusMessage: 'Not Found',
            message: 'Session not found',
        });
    }

    if (currentUser.id !== findSessionById.userId) {
        throw createError({
            statusCode: 403,
            statusMessage: 'Forbidden',
            message: 'You do not have permission to perform this action',
        });
    }

    await prisma.session.delete({
        where: {
            id: sessionId,
        },
    });

    await sendToSession(currentUser.id, sessionId!, 'logout', null);
    sendToUser(currentUser.id, 'delete:session', sessionId);
});
