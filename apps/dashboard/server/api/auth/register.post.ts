import { z } from 'zod';

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

export default defineEventHandler(async (event) => {
    const settings = await getSettings();

    if (!settings.registrationEnabled) {
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
                select: { files: true, folders: true, notes: true },
            },
        },
    });

    return startSession(event, findCreatedUser, {
        action: 'Register',
        message: 'Registered',
    });
});
