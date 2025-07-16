import { authenticator } from 'otplib';
import { z } from 'zod';

const validationSchema = z.object({
    enabled: z.boolean(),
    totp: z
        .string()
        .length(6, 'TOTP must be 6 digits')
        .regex(/^\d+$/, 'TOTP must be a number')
        .optional(),
    verificationData: z.any().optional(),
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
            data: { formErrors: body.error.format() },
        });
    }

    if (currentUser.totpEnabled === body.data.enabled) {
        throw createError({
            statusCode: 400,
            statusMessage: 'Bad Request',
            message: `Authenticator App is already ${body.data.enabled ? 'enabled' : 'disabled'}`,
        });
    }

    if (!body.data.enabled) {
        await verifySession(event, body.data.verificationData);
    }

    if (body.data.enabled && !authenticator.check(body.data.totp!, currentUser.totpSecret!)) {
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
        message: `${body.data.enabled ? 'Enabled' : 'Disabled'} Authenticator App`,
    });

    await sendByFilter(isAdmin, 'update:user:totp', body.data.enabled);

    sendToUser(currentUser.id, 'update:currentUser:totp', body.data.enabled);
});
