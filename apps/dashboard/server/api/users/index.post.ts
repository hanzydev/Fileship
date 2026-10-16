import { z } from 'zod';

import { UserPermission } from '#shared/prisma/enums';

const validationSchema = z.object({
    username: z.string().min(3, 'errorsUsers.usernameMin').max(24, 'errorsUsers.usernameMax'),
    password: z.string().min(8, 'errorsUsers.passwordMin').max(48, 'errorsUsers.passwordMax'),
    permissions: z.array(z.enum(UserPermission)).optional(),
    limits: z
        .object({
            backupLimit: z
                .number()
                .int('errorsUsers.backupLimitInt')
                .min(-1, 'errorsUsers.backupLimitMin')
                .optional(),
            usableSpace: z
                .number()
                .int('errorsUsers.usableSpaceInt')
                .min(-1, 'errorsUsers.usableSpaceMin')
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
        throw invalidBodyError(body.error);
    }

    if (body.data.superAdmin && !currentUser.superAdmin) {
        throw apiError(403, 'errorsUsers.cannotCreateSuperAdmin');
    }

    if (!body.data.superAdmin && !body.data.permissions?.length) {
        throw apiError(400, 'errorsUsers.permissionsRequired');
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
        throw apiError(409, 'errorsUsers.usernameExists');
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
