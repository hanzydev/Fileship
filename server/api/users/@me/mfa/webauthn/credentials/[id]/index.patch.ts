import { z } from 'zod';

const validationSchema = z
    .object({
        name: z
            .string({ invalid_type_error: 'Invalid name' })
            .min(3, { message: 'Name must be at least 3 characters' })
            .max(32, { message: 'Name must be less than 32 characters' }),
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
        },
    });

    if (!findCredentialById) {
        throw createError({
            statusCode: 404,
            statusMessage: 'Not Found',
            message: 'Credential not found',
        });
    }

    if (currentUser.id !== findCredentialById.userId) throw forbiddenError;

    const updatedCredential = await prisma.credential.update({
        where: {
            id: credentialId,
        },
        data: {
            name: body.data?.name,
        },
        select: {
            id: true,
            name: true,
            createdAt: true,
        },
    });

    sendToUser(currentUser.id, 'update:passkey', updatedCredential);
});
