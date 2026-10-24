import { z } from 'zod';

const validationSchema = z.object({
    username: z.string().trim().min(3, 'errorsAuth.usernameMin').max(24, 'errorsAuth.usernameMax'),
    password: z.string().min(8, 'errorsAuth.passwordMin').max(48, 'errorsAuth.passwordMax'),
    turnstile: z.string().optional(),
    invite: z.string().min(1).max(128).optional(),
});

export default defineEventHandler(async (event) => {
    const settings = await getSettings();

    if (event.context.user) {
        throw apiError(400, 'errorsAuth.alreadyLoggedIn');
    }

    const body = await readValidatedBody(event, validationSchema.safeParse);
    if (!body.success) throw invalidBodyError(body.error);

    if (!body.data.invite && !settings.registrationEnabled) {
        throw apiError(403, 'errorsAuth.registrationDisabled');
    }

    await rateLimit(`register:${getClientIp(event)}`, 5, 60 * 60 * 1000);

    if (!(await verifyTurnstile(body.data.turnstile))) {
        throw apiError(400, 'errorsAuth.invalidTurnstile');
    }

    const invite = body.data.invite ? await findInvite(body.data.invite) : null;

    if (body.data.invite && !invite) {
        throw apiError(400, 'errorsEmail.invalidInvite');
    }

    const findUserByUsername = await prisma.user.findUnique({
        where: {
            username: body.data.username,
        },
    });

    if (findUserByUsername) {
        throw apiError(409, 'errorsAuth.usernameExists', undefined, {
            formErrors: { username: { _errors: [translate('errorsAuth.usernameTaken')] } },
        });
    }

    if (invite) {
        const { count } = await prisma.invite.deleteMany({
            where: { id: invite.id, expiresAt: { gt: new Date() } },
        });

        if (!count) throw apiError(400, 'errorsEmail.invalidInvite');
    }

    const createdUser = await createUser({
        username: body.data.username,
        password: body.data.password,
        permissions: invite?.permissions ?? settings.registrationPermissions,
        limits: { usableSpace: invite?.usableSpace ?? settings.registrationUsableSpace },
        email:
            invite && !(await prisma.user.count({ where: { email: invite.email } }))
                ? invite.email
                : undefined,
    });

    if (invite) await sendByFilter(isAdmin, 'invite:update', null);

    const findCreatedUser = await prisma.user.findUniqueOrThrow({
        where: {
            id: createdUser.id,
        },
        include: {
            _count: {
                select: userCountSelect,
            },
        },
    });

    return startSession(
        event,
        findCreatedUser,
        invite
            ? {
                  action: 'Register',
                  message: `Registered with an invite from ${invite.invitedBy.username}`,
              }
            : { action: 'Register', message: 'Registered' },
    );
});
