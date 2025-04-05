import { z } from 'zod';

const validationSchema = z
    .object({
        verificationData: z.any().optional(),
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
            data: { formErrors: body.error.format() },
        });
    }

    await verifySession(event, body.data?.verificationData);

    const sessionId = getRouterParam(event, 'id');
    const findSessionById = await prisma.session.findUnique({
        where: {
            id: sessionId,
            userId: currentUser.id,
        },
    });

    if (!findSessionById) {
        throw createError({
            statusCode: 404,
            statusMessage: 'Not Found',
            message: 'Session not found',
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
