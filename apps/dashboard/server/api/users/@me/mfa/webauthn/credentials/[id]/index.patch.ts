import { z } from 'zod';

const validationSchema = z
    .object({
        name: z.string().min(3, 'errorsAuth.nameMin').max(32, 'errorsAuth.nameMax'),
        verificationData: z.any().optional(),
    })
    .optional();

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) throw invalidBodyError(body.error);

    await verifySession(event, body.data?.verificationData);

    const credentialId = getRouterParam(event, 'id');
    const findCredentialById = await prisma.credential.findUnique({
        where: {
            id: credentialId,
            userId: currentUser.id,
        },
    });

    if (!findCredentialById) {
        throw apiError(404, 'errorsAuth.passkeyNotFound');
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

    sendToUser(currentUser.id, 'passkey:update', updatedCredential);
});
