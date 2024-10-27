import { hash } from 'argon2';
import defu from 'defu';
import { z } from 'zod';

import { UserPermission } from '@prisma/client';

import { defaultUserLimits } from '~~/utils/constants';
import { isAdmin } from '~~/utils/permissions';
import type { IUserLimits } from '~~/utils/types';

const validationSchema = z.object(
    {
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
        permissions: z
            .array(z.nativeEnum(UserPermission), {
                invalid_type_error: 'Invalid permissions',
                required_error: 'Missing permissions',
            })
            .optional(),
        limits: z
            .object({
                backupLimit: z
                    .number({
                        invalid_type_error: 'Invalid backup limit',
                        required_error: 'Missing backup limit',
                    })
                    .min(-1, 'Backup limit must be at least -1')
                    .optional(),
                usableSpace: z
                    .number({
                        invalid_type_error: 'Invalid usable space',
                        required_error: 'Missing usable space',
                    })
                    .min(-1, 'Usable space must be at least -1')
                    .optional(),
            })
            .optional(),
        superAdmin: z
            .boolean({
                invalid_type_error: 'Invalid super admin',
            })
            .optional(),
        verificationData: z.any().optional(),
    },
    { invalid_type_error: 'Invalid body', required_error: 'Missing body' },
);

export default defineEventHandler(async (event) => {
    adminOnly(event);

    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw createError({
            statusCode: 400,
            statusMessage: 'Bad Request',
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    if (body.data.superAdmin && !currentUser.superAdmin) {
        throw createError({
            statusCode: 403,
            statusMessage: 'Forbidden',
            message: 'You do not have permission to create a super admin',
        });
    }

    if (!body.data.superAdmin && !body.data.permissions?.length) {
        throw createError({
            statusCode: 400,
            statusMessage: 'Bad Request',
            message:
                'Permissions must be provided if user is not a super admin',
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
            statusMessage: 'Conflict',
            message: 'A user with that username already exists',
        });
    }

    const limits = defu(body.data.limits, defaultUserLimits);

    const user = await prisma.user.create({
        data: {
            username: body.data.username,
            password: await hash(body.data.password),
            permissions: body.data.permissions,
            superAdmin: body.data.superAdmin,
            limits,
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
        },
    });

    await createLog(event, {
        action: 'Create User',
        message: `Created user ${user.username} with permissions ${user.permissions.join(
            ', ',
        )}`,
    });

    await sendByFilter((user) => isAdmin(user), 'create:user', user);

    return {
        ...user,
        limits: limits as IUserLimits,
    };
});
