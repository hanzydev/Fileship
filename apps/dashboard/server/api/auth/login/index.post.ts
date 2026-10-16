import { hash, verify } from 'argon2';
import { z } from 'zod';

const validationSchema = z.object({
    username: z.string().min(3, 'errorsAuth.usernameMin').max(24, 'errorsAuth.usernameMax'),
    password: z.string().min(8, 'errorsAuth.passwordMin').max(48, 'errorsAuth.passwordMax'),
    totp: z
        .string()
        .length(6, 'errorsAuth.totpLength')
        .regex(/^\d+$/, 'errorsAuth.totpNumber')
        .optional(),
    turnstile: z.string().optional(),
});

const dummyHash = hash('fileship-dummy-password');

export default defineEventHandler(async (event) => {
    const body = await readValidatedBody(event, validationSchema.safeParse);
    if (!body.success) throw invalidBodyError(body.error);

    await rateLimit(`login:${getClientIp(event)}:${body.data.username}`, 10, 15 * 60 * 1000);

    if (!(await verifyTurnstile(body.data.turnstile))) {
        throw apiError(400, 'errorsAuth.invalidTurnstile');
    }

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

    const passwordMatch = await verify(
        findUserByUsername?.password ?? (await dummyHash),
        body.data.password,
    );

    if (!findUserByUsername || !passwordMatch) {
        throw apiError(401, 'errorsAuth.invalidCredentials');
    }

    if (findUserByUsername.totpEnabled) {
        if (!body.data.totp) {
            throw apiError(400, 'errorsAuth.totpRequired', undefined, { totpRequired: true });
        }

        if (
            !(await verifyTotpToken(
                findUserByUsername.id,
                findUserByUsername.totpSecret!,
                body.data.totp,
            ))
        ) {
            throw apiError(401, 'errorsAuth.invalidTotp');
        }
    }

    return startSession(event, findUserByUsername, {
        action: 'Login',
        message: 'Logged in',
    });
});
