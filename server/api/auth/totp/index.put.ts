import { authenticator } from 'otplib';
import { z } from 'zod';

import { isAdmin } from '~~/utils/permissions';

const validationSchema = z.object({
    enabled: z.boolean({
        invalid_type_error: 'Invalid enabled',
        required_error: 'Missing enabled',
    }),
    totp: z
        .string({
            invalid_type_error: 'Invalid TOTP',
            required_error: 'Missing TOTP',
        })
        .length(6, 'TOTP must be 6 digits')
        .regex(/^\d+$/, 'TOTP must be a number'),
});

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw createError({
            statusCode: 400,
            statusMessage: 'Bad Request',
            message: 'Invalid body',
            data: body.error.format(),
        });
    }

    if (currentUser.totpEnabled === body.data.enabled) {
        throw createError({
            statusCode: 400,
            statusMessage: 'Bad Request',
            message: `You cannot access the QR Code because the Two-Factor Authentication is already ${body.data.enabled ? 'enabled' : 'disabled'}`,
        });
    }

    if (!authenticator.check(body.data.totp, currentUser.totpSecret!)) {
        throw createError({
            statusCode: 400,
            statusMessage: 'Bad Request',
            message: 'Invalid TOTP',
        });
    }

    await prisma.user.update({
        where: {
            id: currentUser.id,
        },
        data: {
            totpEnabled: body.data.enabled,
        },
    });

    await prisma.session.updateMany({
        where: {
            userId: currentUser.id,
        },
        data: {
            lastVerify: null,
        },
    });

    await createLog(event, {
        action: 'TOTP',
        message: `${body.data.enabled ? 'Enabled' : 'Disabled'} Two-Factor Authentication`,
    });

    await sendByFilter(
        (user) => isAdmin(user),
        'update:user:totp',
        body.data.enabled,
    );

    sendToUser(currentUser.id, 'update:currentUser:totp', body.data.enabled);
});
