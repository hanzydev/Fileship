import { z } from 'zod';

import { UserPermission } from '#shared/prisma/enums';

const validationSchema = z.object({
    username: z
        .string()
        .trim()
        .min(3, 'Username must be at least 3 characters')
        .max(24, 'Username must be at most 24 characters'),
    password: z
        .string()
        .min(8, 'Password must be at least 8 characters')
        .max(48, 'Password must be at most 48 characters'),
    turnstile: z.string().optional(),
});

const registrablePermissions: UserPermission[] = [
    UserPermission.UploadFiles,
    UserPermission.TakeNotes,
];

export default defineEventHandler(async (event) => {
    const runtimeConfig = useRuntimeConfig();

    if (!runtimeConfig.public.registrationEnabled) {
        throw createError({
            status: 403,
            message: 'Registration is disabled',
        });
    }

    if (event.context.user) {
        throw createError({
            status: 400,
            message: 'You are already logged in',
        });
    }

    const body = await readValidatedBody(event, validationSchema.safeParse);
    if (!body.success) {
        throw createError({
            status: 400,
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    if (runtimeConfig.turnstile.secretKey) {
        const turnstileResult = await verifyTurnstileToken(body.data.turnstile!, event);
        if (!turnstileResult.success) {
            throw createError({
                status: 400,
                message: 'Invalid turnstile',
            });
        }
    }

    const findUserByUsername = await prisma.user.findUnique({
        where: {
            username: body.data.username,
        },
    });

    if (findUserByUsername) {
        throw createError({
            status: 409,
            message: 'A user with that username already exists',
            data: {
                formErrors: { username: { _errors: ['This username is already taken'] } },
            },
        });
    }

    const permissions = String(runtimeConfig.registration.permissions)
        .split(',')
        .map((permission) => permission.trim())
        .filter((permission): permission is UserPermission =>
            registrablePermissions.includes(permission as UserPermission),
        );

    const usableSpace = Number(runtimeConfig.registration.usableSpace);

    const createdUser = await createUser({
        username: body.data.username,
        password: body.data.password,
        permissions: [...new Set(permissions)],
        limits: { usableSpace: Number.isFinite(usableSpace) ? Math.max(usableSpace, -1) : -1 },
    });

    const findCreatedUser = await prisma.user.findUniqueOrThrow({
        where: {
            id: createdUser.id,
        },
        include: {
            _count: {
                select: { files: true, folders: true, notes: true },
            },
        },
    });

    return startSession(event, findCreatedUser, {
        action: 'Register',
        message: 'Registered',
    });
});
