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
    const settings = await getSettings();

    if (
        !currentUser.password &&
        !(await prisma.credential.count({ where: { userId: currentUser.id } }))
    ) {
        throw apiError(400, 'errorsOidc.setPasswordFirst');
    }

    if (!canLogInWithPassword(currentUser, settings)) {
        throw apiError(400, 'errorsOidc.passwordLoginOff');
    }

    const { count } = await prisma.oidcIdentity.deleteMany({ where: { userId: currentUser.id } });
    if (!count) throw apiError(404, 'errorsOidc.notLinked');

    await createLog(event, {
        action: 'Unlink SSO',
        message: 'Unlinked their single sign-on account',
    });
});
