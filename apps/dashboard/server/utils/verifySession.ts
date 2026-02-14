import { verify } from 'argon2';
import type { H3Event } from 'h3';
import { verify as verifyTotp } from 'otplib';

import { base64URLStringToBuffer } from '@simplewebauthn/browser';
import {
    generateAuthenticationOptions,
    verifyAuthenticationResponse,
} from '@simplewebauthn/server';
import type {
    AuthenticationResponseJSON,
    AuthenticatorTransportFuture,
} from '@simplewebauthn/types';

interface PasskeyVerificationData {
    expectedChallenge: string;
    authenticationResponse: AuthenticationResponseJSON;
}

export const verifySession = async (
    event: H3Event,
    verificationData?: {
        type: 'passkey' | 'totp' | 'password';
        data: string | PasskeyVerificationData;
    },
) => {
    const currentUser = event.context.user!;
    const reqUrl = getRequestURL(event);

    const findCurrentSessionById = await prisma.session.findUnique({
        where: {
            id: currentUser.currentSessionId,
        },
    });

    if (
        !findCurrentSessionById!.lastVerify ||
        Date.now() - findCurrentSessionById!.lastVerify.getTime() > 5 * 60 * 1000 /** 5 minutes */
    ) {
        if (!verificationData) {
            const allowCredentials = (await prisma.credential.findMany({
                where: {
                    userId: currentUser.id,
                },
                select: {
                    id: true,
                    transports: true,
                },
            })) as { id: string; transports: AuthenticatorTransportFuture[] }[];

            throw createError({
                statusCode: 400,
                message: 'Verification is required',
                data: {
                    mfa: {
                        methods: [
                            allowCredentials.length && {
                                type: 'passkey',
                                challange: await generateAuthenticationOptions({
                                    rpID: reqUrl.hostname,
                                    allowCredentials,
                                    userVerification: 'required',
                                }),
                            },
                            {
                                type: currentUser.totpEnabled ? 'totp' : 'password',
                            },
                        ].filter(Boolean),
                    },
                },
            });
        }

        if (currentUser.totpEnabled) {
            if (verificationData?.type === 'totp') {
                const totpValid = await verifyTotp({
                    secret: currentUser.totpSecret!,
                    token: verificationData.data as string,
                });

                if (!totpValid.valid) {
                    throw createError({
                        statusCode: 401,
                        message: 'Invalid TOTP',
                    });
                }
            } else if (verificationData?.type === 'password') {
                throw createError({
                    statusCode: 400,
                    message: 'Password verification is not allowed because TOTP is enabled',
                });
            }
        } else if (verificationData?.type === 'password') {
            const passwordMatch = await verify(
                currentUser.password,
                verificationData.data as string,
            );

            if (!passwordMatch) {
                throw createError({
                    statusCode: 401,
                    message: 'Invalid password',
                });
            }
        } else if (verificationData?.type === 'passkey') {
            const findCredentialById = (await prisma.credential.findUnique({
                where: {
                    id: (verificationData.data as PasskeyVerificationData).authenticationResponse
                        .id,
                },
            }))!;

            const response = await verifyAuthenticationResponse({
                response: (verificationData.data as PasskeyVerificationData).authenticationResponse,
                expectedChallenge: (verificationData.data as PasskeyVerificationData)
                    .expectedChallenge,
                expectedOrigin: reqUrl.origin,
                expectedRPID: reqUrl.hostname,
                credential: {
                    id: findCredentialById.id,
                    publicKey: new Uint8Array(
                        base64URLStringToBuffer(findCredentialById.publicKey),
                    ),
                    counter: findCredentialById.counter,
                    transports: findCredentialById.transports as AuthenticatorTransportFuture[],
                },
                requireUserVerification: true,
            });

            if (!response.verified) {
                throw createError({
                    statusCode: 401,
                    message: 'Invalid passkey',
                });
            }
        } else {
            throw createError({
                statusCode: 400,
                message: 'Invalid verification type',
            });
        }

        await prisma.session.update({
            where: {
                id: currentUser.currentSessionId,
            },
            data: {
                lastVerify: new Date(),
            },
        });
    }
};
