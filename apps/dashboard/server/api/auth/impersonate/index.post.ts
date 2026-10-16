import { z } from 'zod';

const validationSchema = z.object({
    username: z.string(),
    verificationData: z.any().optional(),
});

export default defineEventHandler(async (event) => {
    adminOnly(event);

    const currentUser = event.context.user!;

    const body = await readValidatedBody(event, validationSchema.safeParse);
    if (!body.success) throw invalidBodyError(body.error);

    const findUserByUsername = await prisma.user.findUnique({
        where: {
            username: body.data.username,
        },
        include: {
            _count: {
                select: userCountSelect,
            },
        },
    });

    if (
        !findUserByUsername ||
        findUserByUsername.id === currentUser.id ||
        (findUserByUsername.superAdmin && !currentUser.superAdmin) ||
        currentUser.impersonator
    ) {
        throw apiError(403, 'errorsAuth.cannotImpersonate');
    }

    await verifySession(event, body.data.verificationData);

    setTokenCookie(event, 'adminToken', getCookie(event, 'token')!);

    return startSession(
        event,
        findUserByUsername,
        {
            action: 'Impersonate',
            message: `Signed in by ${currentUser.username}`,
        },
        currentUser.currentSessionId,
    );
});
