import { verify } from 'argon2';
import type { H3Event } from 'h3';
import { nanoid } from 'nanoid';
import { verify as verifyTotp } from 'otplib';

import { base64URLStringToBuffer } from '@simplewebauthn/browser';
import {
    generateAuthenticationOptions,
    verifyAuthenticationResponse,
} from '@simplewebauthn/server';
import type {
    AuthenticationResponseJSON,
    AuthenticatorTransportFuture,
    PublicKeyCredentialRequestOptionsJSON,
} from '@simplewebauthn/types';

interface PasskeyVerificationData {
    ticket: string;
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
    const storage = useStorage('cache');

    const currentSession = await prisma.session.findUnique({
        where: {
            id: currentUser.currentSessionId,
        },
    });
    if (!currentSession) {
        throw createError({
            statusCode: 401,
            message: 'Session not found',
        });
    }

    if (
        !currentSession.lastVerify ||
        Date.now() - currentSession.lastVerify.getTime() > 5 * 60 * 1000 /** 5 minutes */
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

            let passkeyMethod: {
                ticket: string;
                authenticationOptions: PublicKeyCredentialRequestOptionsJSON;
            } | null = null;
            if (allowCredentials.length) {
                const ticket = nanoid(32);
                const authenticationOptions = await generateAuthenticationOptions({
                    rpID: reqUrl.hostname,
                    allowCredentials,
                    userVerification: 'required',
                });

                passkeyMethod = {
                    ticket,
                    authenticationOptions,
                };

                await storage.setItem(`webauthnTicket:${ticket}`, {
                    expectedChallenge: authenticationOptions.challenge,
                    expiresAt: Date.now() + 5 * 60 * 1000, // 5 minutes
                });
            }

            throw createError({
                statusCode: 400,
                message: 'Verification is required',
                data: {
                    mfa: {
                        methods: [
                            passkeyMethod && {
                                type: 'passkey',
                                ...passkeyMethod,
                            },
                            {
                                type: currentUser.totpEnabled ? 'totp' : 'password',
                            },
                        ].filter(Boolean),
                    },
                },
            });
        }

        if (verificationData?.type === 'totp') {
            if (!currentUser.totpEnabled) {
                throw createError({
                    statusCode: 400,
                    message: 'TOTP verification is not enabled for this user',
                });
            }

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
            if (currentUser.totpEnabled) {
                throw createError({
                    statusCode: 400,
                    message: 'Password verification is not allowed because TOTP is enabled',
                });
            }

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
            const { ticket, authenticationResponse } =
                verificationData.data as PasskeyVerificationData;

            const findCredentialById = await prisma.credential.findUnique({
                where: {
                    id: authenticationResponse.id,
                    userId: currentUser.id,
                },
            });

            if (!findCredentialById) {
                throw createError({
                    statusCode: 401,
                    message: 'Credential not found',
                });
            }

            const webauthnTicket = await storage.getItem<{
                expectedChallenge: string;
                expiresAt: number;
            }>(`webauthnTicket:${ticket}`);
            if (!webauthnTicket || webauthnTicket.expiresAt < Date.now()) {
                throw createError({
                    statusCode: 400,
                    message: 'Invalid or expired ticket',
                });
            }

            const expectedChallenge = webauthnTicket.expectedChallenge;
            await storage.removeItem(`webauthnTicket:${ticket}`);

            const response = await verifyAuthenticationResponse({
                response: authenticationResponse,
                expectedChallenge,
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

            await prisma.credential.update({
                where: {
                    id: findCredentialById.id,
                },
                data: {
                    counter: response.authenticationInfo.newCounter,
                },
            });
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
