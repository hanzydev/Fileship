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

    if (!body.success) throw invalidBodyError(body.error);

    if (currentUser.totpEnabled) {
        throw apiError(400, 'errorsAuth.totpAlreadyEnabled');
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

    const { siteName } = await getSettings();

    return {
        base64: `data:image/svg+xml;base64,${Buffer.from(
            renderSVG(
                generateURI({
                    secret: totpSecret,
                    label: currentUser.username,
                    issuer: siteName,
                }),
            ),
        ).toString('base64')}`,
    };
});
