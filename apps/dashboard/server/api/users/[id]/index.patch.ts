import { hash } from 'argon2';
import { defu } from 'defu';
import sharp, { type Metadata, type Sharp } from 'sharp';
import { z } from 'zod';

import { update } from '@orama/orama';

import { UserPermission } from '#shared/prisma/enums';

const validationSchema = z
    .object({
        username: z
            .string()
            .min(3, 'errorsUsers.usernameMin')
            .max(24, 'errorsUsers.usernameMax')
            .optional(),
        avatar: z.string().nullish(),
        password: z
            .string()
            .min(8, 'errorsUsers.passwordMin')
            .max(48, 'errorsUsers.passwordMax')
            .optional(),
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
        verificationData: z.any().nullish(),
        theme: z.enum(Object.keys(accents) as [Accent]).optional(),
        language: z.enum(['en', 'tr']).nullable().optional(),
    })
    .strict();

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const userId = getRouterParam(event, 'id');

    if (userId !== currentUser.id && !isAdmin(currentUser)) {
        throw forbiddenError();
    }

    const findUserById = await prisma.user.findUnique({
        where: {
            id: userId,
        },
    });

    if (!findUserById) {
        throw apiError(404, 'errorsUsers.userNotFound');
    }

    if (findUserById.superAdmin && !currentUser.superAdmin) {
        throw apiError(403, 'errorsUsers.cannotUpdateSuperAdmin');
    }

    const body = await readValidatedBody(event, validationSchema.safeParse);
    if (!body.success) {
        throw invalidBodyError(body.error);
    }

    if (
        !isAdmin(currentUser) &&
        ('permissions' in body.data || 'limits' in body.data || 'superAdmin' in body.data)
    ) {
        throw forbiddenError();
    }

    if ('superAdmin' in body.data && body.data.superAdmin && !currentUser.superAdmin) {
        throw apiError(403, 'errorsUsers.cannotGrantSuperAdmin');
    }

    if (body.data.permissions && !body.data.permissions.length && !body.data.superAdmin) {
        throw apiError(400, 'errorsUsers.permissionsRequired');
    }

    if (body.data.username) {
        const findUserByUsername = await prisma.user.findFirst({
            where: {
                username: body.data.username,
                id: { not: userId },
            },
        });

        if (findUserByUsername) {
            throw apiError(409, 'errorsUsers.usernameTaken', undefined, {
                formErrors: {
                    username: {
                        _errors: [translate('errorsUsers.usernameTaken')],
                    },
                },
            });
        }
    }

    if (body.data.superAdmin) body.data.permissions = [UserPermission.Admin];

    const grantsAdmin =
        (body.data.superAdmin && !findUserById.superAdmin) ||
        (body.data.permissions?.includes(UserPermission.Admin) &&
            !findUserById.permissions.includes(UserPermission.Admin));

    if (grantsAdmin) await verifySession(event, body.data.verificationData);

    if (body.data.avatar) {
        const buffer = Buffer.from(body.data.avatar, 'base64');

        const { avatarMaxSize } = await getSettings();

        if (buffer.length > avatarMaxSize * 1024 * 1024) {
            throw apiError(400, 'errorsUsers.avatarTooLarge', { size: avatarMaxSize });
        }

        let image: Sharp;
        let metadata: Metadata;

        try {
            image = sharp(buffer);
            metadata = await image.metadata();
        } catch {
            throw apiError(400, 'errorsUsers.invalidAvatarImage');
        }

        if (!metadata.width || !metadata.height) {
            throw apiError(400, 'errorsUsers.invalidAvatar');
        }

        if (metadata.width < 64 || metadata.height < 64) {
            throw apiError(400, 'errorsUsers.avatarTooSmall');
        }

        try {
            const resizedAvatar = await image
                .rotate()
                .toFormat('png', { quality: 80 })
                .resize(64, 64)
                .toBuffer();

            body.data.avatar = `data:image/png;base64,${resizedAvatar.toString('base64')}`;
        } catch {
            throw apiError(400, 'errorsUsers.avatarProcessFailed');
        }
    }

    if (body.data.password) {
        await verifySession(event, body.data.verificationData!);

        (body.data as any).password = await hash(body.data.password);

        await prisma.session.deleteMany({
            where: {
                userId,
                id: {
                    not: currentUser.currentSessionId,
                },
            },
        });

        await logoutSockets((user) =>
            user.id === userId
                ? userId === currentUser.id
                    ? user.currentSessionId !== currentUser.currentSessionId
                    : true
                : false,
        );

        sendToUser(userId!, 'session:deleteAll', null);
    } else delete body.data.password;

    delete body.data.verificationData;

    if (body.data.limits) {
        body.data.limits = defu(
            body.data.limits,
            findUserById.limits as Partial<IUserLimits>,
            defaultUserLimits,
        );
    }

    const _updatedUser = await prisma.$transaction(async (tx) => {
        const superAdminsCount = await tx.user.count({
            where: { superAdmin: true },
        });

        if (
            findUserById.id === currentUser.id &&
            findUserById.superAdmin &&
            'superAdmin' in body.data &&
            !body.data.superAdmin &&
            superAdminsCount <= 1
        ) {
            throw apiError(403, 'errorsUsers.lastSuperAdmin');
        }

        return await tx.user.update({
            where: { id: userId },
            data: body.data,
            select: {
                id: true,
                username: true,
                avatar: true,
                permissions: true,
                totpEnabled: true,
                createdAt: true,
                limits: true,
                superAdmin: true,
                theme: true,
                language: true,
                aiSettings: true,
                _count: {
                    select: userCountSelect,
                },
            },
        });
    });

    const updatedUser = {
        ..._updatedUser,
        _count: undefined,
        stats: _updatedUser._count,
        limits: defu(_updatedUser.limits, defaultUserLimits) as IUserLimits | undefined,
        aiSettings: defu(_updatedUser.aiSettings, defaultUserAiSettings) as IUserAiSettings,
    };

    await update(userSearchDb, updatedUser.id, {
        id: updatedUser.id,
        username: updatedUser.username,
    });

    await createLog(event, {
        action: 'Update User',
        message: `Updated user ${updatedUser.username}`,
    });

    await refreshSocketUser(updatedUser.id, {
        permissions: updatedUser.permissions,
        superAdmin: updatedUser.superAdmin,
    });

    await sendByFilter(isAdmin, 'user:update', { ...updatedUser, aiSettings: undefined });

    updatedUser.limits = undefined;

    sendToUser(updatedUser.id, 'currentUser:update', updatedUser);

    return updatedUser;
});
