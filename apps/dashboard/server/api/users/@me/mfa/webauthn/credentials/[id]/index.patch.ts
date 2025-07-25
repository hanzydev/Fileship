import { z } from 'zod';

const validationSchema = z
    .object({
        name: z
            .string()
            .min(3, 'Name must be at least 3 characters')
            .max(32, 'Name must be less than 32 characters'),
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
            message: 'Credential not found',
        });
    }

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
