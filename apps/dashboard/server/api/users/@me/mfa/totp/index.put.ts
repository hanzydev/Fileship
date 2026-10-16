import { verify as verifyTotp } from 'otplib';
import { z } from 'zod';

const validationSchema = z.object({
    enabled: z.boolean(),
    totp: z
        .string()
        .length(6, 'errorsAuth.totpLength')
        .regex(/^\d+$/, 'errorsAuth.totpNumber')
        .optional(),
    verificationData: z.any().optional(),
});

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) throw invalidBodyError(body.error);

    if (currentUser.totpEnabled === body.data.enabled) {
        throw apiError(
            400,
            body.data.enabled ? 'errorsAuth.totpAlreadyEnabled' : 'errorsAuth.totpAlreadyDisabled',
        );
    }

    if (!body.data.enabled) {
        await verifySession(event, body.data.verificationData);
    }

    if (
        body.data.enabled &&
        !(await verifyTotp({ secret: currentUser.totpSecret!, token: body.data.totp! })).valid
    ) {
        throw apiError(400, 'errorsAuth.invalidTotp');
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

    await sendByFilter(isAdmin, 'user:updateTotp', {
        id: currentUser.id,
        totpEnabled: body.data.enabled,
    });

    sendToUser(currentUser.id, 'currentUser:updateTotp', body.data.enabled);
});
