import { authenticator } from 'otplib';
import qrcode from 'qrcode';
import { z } from 'zod';

const validationSchema = z
    .object({
        verificationData: z
            .string({
                invalid_type_error: 'Invalid verification data',
            })
            .optional(),
    })
    .optional();

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;

    if (!currentUser) {
        throw createError({
            statusCode: 401,
            statusMessage: 'Unauthorized',
            message: 'You do not have permission to perform this action',
        });
    }

    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw createError({
            statusCode: 400,
            statusMessage: 'Bad Request',
            message: 'Invalid body',
            data: body.error.format(),
        });
    }

    if (currentUser.totpEnabled) {
        throw createError({
            statusCode: 400,
            statusMessage: 'Bad Request',
            message:
                'You cannot access the QR Code because the Two-Factor Authentication is already switched on',
        });
    }

    await verifySession(currentUser, body.data?.verificationData);

    const totpSecret = authenticator.generateSecret(64);

    await prisma.user.update({
        where: {
            id: currentUser.id,
        },
        data: {
            totpSecret,
        },
    });

    const appConfig = useAppConfig();

    return {
        base64: await qrcode.toDataURL(
            authenticator.keyuri(
                currentUser.username,
                appConfig.site.name,
                totpSecret,
            ),
        ),
    };
});
