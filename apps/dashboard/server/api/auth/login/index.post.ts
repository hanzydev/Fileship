import { verify } from 'argon2';
import { verify as verifyTotp } from 'otplib';
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
    verificationData: z.any().optional(),
});

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;

    const body = await readValidatedBody(event, validationSchema.safeParse);
    if (!body.success) {
        throw createError({
            status: 400,
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    const findUserByUsername = await prisma.user.findUnique({
        where: {
            username: body.data.username,
        },
        include: {
            _count: {
                select: { files: true, folders: true, notes: true },
            },
        },
    });

    if (!findUserByUsername) {
        throw createError({
            status: 401,
            message: 'Invalid username or password',
        });
    }

    const isImpersonating =
        currentUser &&
        isAdmin(currentUser) &&
        (!findUserByUsername?.superAdmin || currentUser.superAdmin);

    if (!isImpersonating) {
        const runtimeConfig = useRuntimeConfig();
        if (runtimeConfig.turnstile.secretKey) {
            const turnstileResult = await verifyTurnstileToken(body.data.turnstile!, event);
            if (!turnstileResult.success) {
                throw createError({
                    status: 400,
                    message: 'Invalid turnstile',
                });
            }
        }

        const passwordMatch = await verify(findUserByUsername.password, body.data.password);

        if (!passwordMatch) {
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

            const totpValid = await verifyTotp({
                secret: findUserByUsername.totpSecret!,
                token: body.data.totp,
            });

            if (!totpValid.valid) {
                throw createError({
                    status: 401,
                    message: 'Invalid TOTP',
                });
            }
        }
    }

    if (currentUser && isAdmin(currentUser) && findUserByUsername.id !== currentUser.id) {
        await verifySession(event, body.data?.verificationData);
    }

    return startSession(event, findUserByUsername, {
        action: 'Login',
        message: 'Logged in',
    });
});
