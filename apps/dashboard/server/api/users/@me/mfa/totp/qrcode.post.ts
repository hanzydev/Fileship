import { generateSecret, generateURI } from 'otplib';
import { renderSVG } from 'uqr';
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
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    if (currentUser.totpEnabled) {
        throw createError({
            statusCode: 400,
            message: 'Authenticator App is already enabled',
        });
    }

    await verifySession(event, body.data?.verificationData);

    const totpSecret = generateSecret({ length: 64 });

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
        base64: `data:image/svg+xml;base64,${Buffer.from(
            renderSVG(
                generateURI({
                    secret: totpSecret,
                    label: currentUser.username,
                    issuer: appConfig.site.name,
                }),
            ),
        ).toString('base64')}`,
    };
});
