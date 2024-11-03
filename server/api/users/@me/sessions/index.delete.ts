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

    await prisma.session.deleteMany({
        where: {
            userId: currentUser.id,
            id: {
                not: currentUser.currentSessionId,
            },
        },
    });

    await sendByFilter(
        (user) =>
            user.id === currentUser.id && user.currentSessionId !== currentUser.currentSessionId,
        'logout',
        null,
    );
    sendToUser(currentUser.id, 'delete:session:all', null);
});
