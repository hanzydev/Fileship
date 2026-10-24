import { z } from 'zod';

const validationSchema = z
    .object({
        verificationData: z.any().optional(),
    })
    .optional();

export default defineEventHandler(async (event) => {
    userOnly(event);

    const body = await readValidatedBody(event, validationSchema.safeParse);
    if (!body.success) throw invalidBodyError(body.error);

    await verifySession(event, body.data?.verificationData);

    const currentUser = event.context.user!;

    await prisma.emailToken.deleteMany({ where: { userId: currentUser.id } });

    await createLog(event, {
        action: 'Remove Email',
        message: 'Removed their email address',
    });

    return updateUserEmail(currentUser.id, { email: null, pendingEmail: null });
});
