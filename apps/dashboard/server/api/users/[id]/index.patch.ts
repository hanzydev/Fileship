import { hash } from 'argon2';
import { defu } from 'defu';
import sharp from 'sharp';
import { z } from 'zod';

import { update } from '@orama/orama';

import { UserPermission } from '#shared/prisma/enums';
import themes from '~~/app/styles/themes.json';

const validationSchema = z
    .object({
        username: z
            .string()
            .min(3, 'Username must be at least 3 characters')
            .max(24, 'Username must be at most 24 characters')
            .optional(),
        avatar: z.string().nullish(),
        password: z
            .string()
            .min(8, 'Password must be at least 8 characters')
            .max(48, 'Password must be at most 48 characters')
            .optional(),
        permissions: z.array(z.enum(UserPermission)).optional(),
        limits: z
            .object({
                backupLimit: z.number().min(-1, 'Backup limit must be at least -1').optional(),
                usableSpace: z.number().min(-1, 'Usable space must be at least -1').optional(),
            })
            .optional(),
        superAdmin: z.boolean().optional(),
        verificationData: z.any().nullish(),
        theme: z
            .string()
            .refine((theme) => theme in themes, 'Invalid theme')
            .optional(),
    })
    .strict();

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const userId = getRouterParam(event, 'id');

    if (userId !== currentUser.id && !isAdmin(currentUser)) {
        throw forbiddenError;
    }

    const findUserById = await prisma.user.findUnique({
        where: {
            id: userId,
        },
    });

    if (!findUserById) {
        throw createError({
            statusCode: 404,
            message: 'User not found',
        });
    }

    if (findUserById.superAdmin && !currentUser.superAdmin) {
        throw createError({
            statusCode: 403,
            message: 'You cannot update a super admin',
        });
    }

    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw createError({
            statusCode: 400,
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    if (
        !isAdmin(currentUser) &&
        ('permissions' in body.data || 'limits' in body.data || 'superAdmin' in body.data)
    ) {
        throw forbiddenError;
    }

    if ('superAdmin' in body.data && body.data.superAdmin && !currentUser.superAdmin) {
        throw createError({
            statusCode: 403,
            message: 'You cannot grant super admin rights',
        });
    }

    const superAdmins = await prisma.user.count({
        where: {
            superAdmin: true,
        },
    });

    if (
        findUserById.id === currentUser.id &&
        findUserById.superAdmin &&
        'superAdmin' in body.data &&
        !body.data.superAdmin &&
        superAdmins === 1
    ) {
        throw createError({
            statusCode: 403,
            message: 'You cannot remove super admin from yourself if you are the only super admin',
        });
    }

    if (body.data.permissions && !body.data.permissions.length && !body.data.superAdmin) {
        throw createError({
            statusCode: 400,
            message: 'Permissions must be provided if user is not a super admin',
        });
    }

    if (body.data.superAdmin) body.data.permissions = [UserPermission.Admin];

    if (body.data.avatar) {
        const buffer = Buffer.from(body.data.avatar, 'base64');

        const avatarMaxSize = +(process.env.AVATAR_MAX_SIZE || 4);

        if (buffer.length > avatarMaxSize * 1024 * 1024) {
            throw createError({
                statusCode: 400,
                message: `Avatar must be less than ${avatarMaxSize}MB`,
            });
        }

        try {
            // eslint-disable-next-line no-var
            var image = sharp(buffer);
            // eslint-disable-next-line no-var
            var metadata = await image.metadata();
        } catch {
            throw createError({
                statusCode: 400,
                message: 'Invalid avatar',
            });
        }

        if (!metadata.width || !metadata.height) {
            throw createError({
                statusCode: 400,
                message: 'Invalid avatar',
            });
        }

        if (metadata.width < 64 || metadata.height < 64) {
            throw createError({
                statusCode: 400,
                message: 'Avatar must be at least 64x64',
            });
        }

        try {
            const resizedAvatar = await image
                .rotate()
                .toFormat('png', { quality: 80 })
                .resize(64, 64)
                .toBuffer();

            body.data.avatar = `data:image/png;base64,${resizedAvatar.toString('base64')}`;
        } catch {
            throw createError({
                statusCode: 400,
                message: 'Failed to process avatar',
            });
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

        await sendByFilter(
            (user) =>
                user.id === userId
                    ? userId === currentUser.id
                        ? user.currentSessionId !== currentUser.currentSessionId
                        : true
                    : false,
            'logout',
            null,
        );

        sendToUser(userId!, 'delete:session:all', null);
    } else delete body.data.password;

    delete body.data.verificationData;

    const updatedUser = await prisma.user.update({
        where: {
            id: userId,
        },
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
        },
    });

    await update(userSearchDb, updatedUser.id, {
        id: updatedUser.id,
        username: updatedUser.username,
    });

    await createLog(event, {
        action: 'Update User',
        message: `Updated user ${updatedUser.username}`,
    });

    await sendByFilter(isAdmin, 'update:user', updatedUser);

    sendToUser(updatedUser.id, 'update:currentUser', updatedUser);

    return {
        ...updatedUser,
        limits: defu(updatedUser.limits, defaultUserLimits) as IUserLimits,
    };
});
