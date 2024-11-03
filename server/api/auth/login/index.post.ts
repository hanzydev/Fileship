import { verify } from 'argon2';
import { nanoid } from 'nanoid';
import { authenticator } from 'otplib';
import { z } from 'zod';

import { isAdmin } from '~~/utils/permissions';

const validationSchema = z.object({
    username: z
        .string({
            invalid_type_error: 'Invalid username',
            required_error: 'Missing username',
        })
        .min(3, 'Username must be at least 3 characters')
        .max(24, 'Username must be at most 24 characters'),
    password: z
        .string({
            invalid_type_error: 'Invalid password',
            required_error: 'Missing password',
        })
        .min(8, 'Password must be at least 8 characters')
        .max(48, 'Password must be at most 48 characters'),
    totp: z
        .string({
            invalid_type_error: 'Invalid TOTP',
        })
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
            statusCode: 400,
            statusMessage: 'Bad Request',
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    const findUserByUsername = await prisma.user.findUnique({
        where: {
            username: body.data.username,
        },
    });

    if (!findUserByUsername) {
        throw createError({
            statusCode: 401,
            statusMessage: 'Unauthorized',
            message: 'Invalid username or password',
        });
    }

    if (
        !(currentUser
            ? isAdmin(currentUser) &&
              (findUserByUsername.superAdmin && !currentUser.superAdmin ? false : true)
            : false)
    ) {
        const runtimeConfig = useRuntimeConfig();
        if (runtimeConfig.turnstile.secretKey) {
            const turnstileResult = await verifyTurnstileToken(body.data.turnstile!, event);
            if (!turnstileResult.success) {
                throw createError({
                    statusCode: 400,
                    statusMessage: 'Bad Request',
                    message: 'Invalid turnstile',
                });
            }
        }

        const passwordMatch = await verify(findUserByUsername.password, body.data.password);

        if (!passwordMatch) {
            throw createError({
                statusCode: 401,
                statusMessage: 'Unauthorized',
                message: 'Invalid username or password',
            });
        }

        if (findUserByUsername.totpEnabled) {
            if (!body.data.totp) {
                throw createError({
                    statusCode: 400,
                    statusMessage: 'Bad Request',
                    message: 'Missing TOTP',
                });
            }

            if (!authenticator.check(body.data.totp, findUserByUsername.totpSecret!)) {
                throw createError({
                    statusCode: 401,
                    statusMessage: 'Unauthorized',
                    message: 'Invalid TOTP',
                });
            }
        }
    }

    if (currentUser && isAdmin(currentUser) && findUserByUsername.id !== currentUser.id) {
        await verifySession(event, body.data?.verificationData);
    }

    const headers = getHeaders(event);
    const ip = getRequestIP(event, { xForwardedFor: true }) || 'Unknown';

    const { os, platform, location } = await getDevice(headers as never, ip);

    const sessionPrivateId = nanoid(128);

    const user = await prisma.user.update({
        where: {
            id: findUserByUsername.id,
        },
        data: {
            sessions: {
                create: {
                    privateId: sessionPrivateId,
                    ip,
                    os,
                    platform,
                    location,
                },
            },
        },
        select: {
            sessions: true,
        },
    });

    const session = user.sessions.find((session) => session.privateId === sessionPrivateId)!;

    await createLog(Object.assign({}, event, { context: { user: findUserByUsername } }), {
        action: 'Login',
        message: `Logged in using ${os} on ${platform}`,
    });

    setCookie(event, 'sessionId', sessionPrivateId, {
        expires: new Date(Date.now() + 1000 * 60 * 60 * 24 * 365),
        path: '/',
        sameSite: 'lax',
    });

    sendToUser(findUserByUsername.id, 'create:session', session);

    return {
        user: {
            id: findUserByUsername.id,
            username: findUserByUsername.username,
            avatar: findUserByUsername.avatar,
            permissions: findUserByUsername.permissions,
            createdAt: findUserByUsername.createdAt,
            totpEnabled: findUserByUsername.totpEnabled,
            superAdmin: findUserByUsername.superAdmin,
            limits: findUserByUsername.limits,
            backupRestoreState: findUserByUsername.backupRestoreState,
            theme: findUserByUsername.theme,
        },
        session: {
            id: session.id,
            privateId: session.privateId,
        },
    };
});
