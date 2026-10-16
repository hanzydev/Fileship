import { z } from 'zod';

const validationSchema = z.object({
    username: z.string().trim().min(3, 'errorsAuth.usernameMin').max(24, 'errorsAuth.usernameMax'),
    password: z.string().min(8, 'errorsAuth.passwordMin').max(48, 'errorsAuth.passwordMax'),
    turnstile: z.string().optional(),
});

export default defineEventHandler(async (event) => {
    const settings = await getSettings();

    if (!settings.registrationEnabled) {
        throw apiError(403, 'errorsAuth.registrationDisabled');
    }

    if (event.context.user) {
        throw apiError(400, 'errorsAuth.alreadyLoggedIn');
    }

    const body = await readValidatedBody(event, validationSchema.safeParse);
    if (!body.success) throw invalidBodyError(body.error);

    await rateLimit(`register:${getClientIp(event)}`, 5, 60 * 60 * 1000);

    if (!(await verifyTurnstile(body.data.turnstile))) {
        throw apiError(400, 'errorsAuth.invalidTurnstile');
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

    const createdUser = await createUser({
        username: body.data.username,
        password: body.data.password,
        permissions: settings.registrationPermissions,
        limits: { usableSpace: settings.registrationUsableSpace },
    });

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

    return startSession(event, findCreatedUser, {
        action: 'Register',
        message: 'Registered',
    });
});
