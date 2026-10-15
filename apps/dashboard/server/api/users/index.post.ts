import { z } from 'zod';

import { UserPermission } from '#shared/prisma/enums';

const validationSchema = z.object({
    username: z
        .string()
        .min(3, 'Username must be at least 3 characters')
        .max(24, 'Username must be at most 24 characters'),
    password: z
        .string()
        .min(8, 'Password must be at least 8 characters')
        .max(48, 'Password must be at most 48 characters'),
    permissions: z.array(z.enum(UserPermission)).optional(),
    limits: z
        .object({
            backupLimit: z
                .number()
                .int('Backup limit must be a whole number')
                .min(-1, 'Backup limit must be at least -1')
                .optional(),
            usableSpace: z
                .number()
                .int('Usable space must be a whole number')
                .min(-1, 'Usable space must be at least -1')
                .optional(),
        })
        .optional(),
    superAdmin: z.boolean().optional(),
    verificationData: z.any().optional(),
});

export default defineEventHandler(async (event) => {
    adminOnly(event);

    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw createError({
            status: 400,
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    if (body.data.superAdmin && !currentUser.superAdmin) {
        throw createError({
            status: 403,
            message: 'You do not have permission to create a super admin',
        });
    }

    if (!body.data.superAdmin && !body.data.permissions?.length) {
        throw createError({
            status: 400,
            message: 'Permissions must be provided if user is not a super admin',
        });
    }

    if (body.data.superAdmin) body.data.permissions = [UserPermission.Admin];

    if (body.data.permissions?.includes(UserPermission.Admin)) {
        await verifySession(event, body.data?.verificationData);
    }

    const findUserByUsername = await prisma.user.findFirst({
        where: {
            username: body.data.username,
        },
    });

    if (findUserByUsername) {
        throw createError({
            status: 409,
            message: 'A user with that username already exists',
        });
    }

    const createdUser = await createUser({
        username: body.data.username,
        password: body.data.password,
        permissions: body.data.permissions,
        superAdmin: body.data.superAdmin,
        limits: body.data.limits,
    });

    await createLog(event, {
        action: 'Create User',
        message: `Created user ${createdUser.username} with permissions ${createdUser.permissions.join(', ')}`,
    });

    return createdUser;
});
