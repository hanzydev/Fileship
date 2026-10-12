import { hash, verify } from 'argon2';
import { z } from 'zod';

const validationSchema = z.object({
    username: z
        .string()
        .min(3, 'Username must be at least 3 characters')
        .max(24, 'Username must be at most 24 characters'),
    password: z
        .string()
        .min(8, 'Password must be at least 8 characters')
        .max(48, 'Password must be at most 48 characters'),
    totp: z
        .string()
        .length(6, 'TOTP must be 6 digits')
        .regex(/^\d+$/, 'TOTP must be a number')
        .optional(),
    turnstile: z.string().optional(),
});

const dummyHash = hash('fileship-dummy-password');

export default defineEventHandler(async (event) => {
    const body = await readValidatedBody(event, validationSchema.safeParse);
    if (!body.success) {
        throw createError({
            status: 400,
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    await rateLimit(
        `login:${getRequestIP(event, { xForwardedFor: true })}:${body.data.username}`,
        10,
        15 * 60 * 1000,
    );

    if (!(await verifyTurnstile(body.data.turnstile))) {
        throw createError({
            status: 400,
            message: 'Invalid turnstile',
        });
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
        throw createError({
            status: 401,
            message: 'Invalid username or password',
        });
    }

    if (findUserByUsername.totpEnabled) {
        if (!body.data.totp) {
            throw createError({
                status: 400,
                message: 'Missing TOTP',
            });
        }

        if (
            !(await verifyTotpToken(
                findUserByUsername.id,
                findUserByUsername.totpSecret!,
                body.data.totp,
            ))
        ) {
            throw createError({
                status: 401,
                message: 'Invalid TOTP',
            });
        }
    }

    return startSession(event, findUserByUsername, {
        action: 'Login',
        message: 'Logged in',
    });
});
