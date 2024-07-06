import { verify } from 'argon2';
import { authenticator } from 'otplib';

export const verifySession = async (
    user: {
        currentSessionId: string;
        password: string;
        totpEnabled: boolean;
        totpSecret: string | null;
    },
    verificationData?: string,
) => {
    const findCurrentSessionById = await prisma.session.findUnique({
        where: {
            id: user.currentSessionId,
        },
    });

    if (
        !findCurrentSessionById!.lastVerify ||
        Date.now() - findCurrentSessionById!.lastVerify.getTime() >
            5 * 60 * 1000
    ) {
        if (!verificationData) {
            throw createError({
                statusCode: 400,
                statusMessage: 'Bad Request',
                message: 'Verification is required',
            });
        }

        if (user.totpEnabled) {
            if (!authenticator.check(verificationData, user.totpSecret!)) {
                throw createError({
                    statusCode: 401,
                    statusMessage: 'Unauthorized',
                    message: 'Invalid TOTP',
                });
            }
        } else {
            const passwordMatch = await verify(user.password, verificationData);

            if (!passwordMatch) {
                throw createError({
                    statusCode: 401,
                    statusMessage: 'Unauthorized',
                    message: 'Invalid password',
                });
            }
        }

        await prisma.session.update({
            where: {
                id: user.currentSessionId,
            },
            data: {
                lastVerify: new Date(),
            },
        });
    }
};
