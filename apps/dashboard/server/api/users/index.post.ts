import { hash } from 'argon2';
import defu from 'defu';
import { z } from 'zod';

import { insert } from '@orama/orama';

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
            backupLimit: z.number().min(-1, 'Backup limit must be at least -1').optional(),
            usableSpace: z.number().min(-1, 'Usable space must be at least -1').optional(),
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
            statusCode: 400,
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    if (body.data.superAdmin && !currentUser.superAdmin) {
        throw createError({
            statusCode: 403,
            message: 'You do not have permission to create a super admin',
        });
    }

    if (!body.data.superAdmin && !body.data.permissions?.length) {
        throw createError({
            statusCode: 400,
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
            statusCode: 409,
            message: 'A user with that username already exists',
        });
    }

    const _createdUser = await prisma.user.create({
        data: {
            username: body.data.username,
            password: await hash(body.data.password),
            permissions: body.data.permissions,
            superAdmin: body.data.superAdmin,
            limits: defu(body.data.limits, defaultUserLimits),
        },
        select: {
            id: true,
            username: true,
            avatar: true,
            permissions: true,
            totpEnabled: true,
            createdAt: true,
            limits: true,
            superAdmin: true,
            _count: {
                select: { files: true, folders: true, notes: true },
            },
        },
    });

    const createdUser = {
        ..._createdUser,
        _count: undefined,
        stats: _createdUser._count,
        limits: defu(_createdUser.limits, defaultUserLimits) as IUserLimits,
    };

    await insert(userSearchDb, {
        id: createdUser.id,
        username: createdUser.username,
    });

    await createLog(event, {
        action: 'Create User',
        message: `Created user ${createdUser.username} with permissions ${createdUser.permissions.join(', ')}`,
    });

    await sendByFilter(isAdmin, 'create:user', createdUser);

    return createdUser;
});
