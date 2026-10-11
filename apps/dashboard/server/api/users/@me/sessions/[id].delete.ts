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
            status: 400,
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
            status: 404,
            message: 'Session not found',
        });
    }

    await prisma.session.delete({
        where: {
            id: sessionId,
        },
    });

    await logoutSockets(
        (user) => user.id === currentUser.id && user.currentSessionId === sessionId,
    );
    sendToUser(currentUser.id, 'session:delete', sessionId);
});
