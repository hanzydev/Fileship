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

    const credentialId = getRouterParam(event, 'id');
    const findCredentialById = await prisma.credential.findUnique({
        where: {
            id: credentialId,
            userId: currentUser.id,
        },
    });

    if (!findCredentialById) {
        throw createError({
            statusCode: 404,
            statusMessage: 'Not Found',
            message: 'Credential not found',
        });
    }

    await prisma.credential.delete({
        where: {
            id: credentialId,
        },
    });

    sendToUser(currentUser.id, 'delete:passkey', credentialId);
});
