import { authenticator } from 'otplib';
import qrcode from 'qrcode';
import { z } from 'zod';

const validationSchema = z
    .object({
        verificationData: z.any().optional(),
    })
    .optional();

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

    if (currentUser.totpEnabled) {
        throw createError({
            statusCode: 400,
            statusMessage: 'Bad Request',
            message: 'Authenticator App is already enabled',
        });
    }

    await verifySession(event, body.data?.verificationData);

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
