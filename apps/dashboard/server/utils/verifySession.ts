import { verify } from 'argon2';
import type { H3Event } from 'h3';
import { nanoid } from 'nanoid';

import { base64URLStringToBuffer } from '@simplewebauthn/browser';
import {
    type AuthenticationResponseJSON,
    generateAuthenticationOptions,
    type PublicKeyCredentialRequestOptionsJSON,
    verifyAuthenticationResponse,
} from '@simplewebauthn/server';

interface PasskeyVerificationData {
    ticket: string;
    authenticationResponse: AuthenticationResponseJSON;
}

export const verifySession = async (
    event: H3Event,
    verificationData?: {
        type: 'passkey' | 'totp' | 'password' | 'oidc';
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
        throw apiError(401, 'errorsAuth.sessionNotFound');
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
            })) as { id: string; transports: string[] }[];

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

            const settings = await getSettings();
            const oidcLinked =
                isOidcConfigured(settings) && !!(await findOidcIdentity(currentUser.id, settings));

            throw apiError(400, 'errorsAuth.verificationRequired', undefined, {
                mfa: {
                    methods: [
                        passkeyMethod && {
                            type: 'passkey',
                            ...passkeyMethod,
                        },
                        currentUser.totpEnabled
                            ? { type: 'totp' }
                            : currentUser.password && { type: 'password' },
                        oidcLinked && { type: 'oidc' },
                    ].filter(Boolean),
                },
            });
        }

        await rateLimit(`verify:${currentUser.currentSessionId}`, 10, 15 * 60 * 1000);

        if (verificationData.type === 'totp') {
            if (!currentUser.totpEnabled) {
                throw apiError(400, 'errorsAuth.totpNotEnabled');
            }

            if (
                !(await verifyTotpToken(
                    currentUser.id,
                    currentUser.totpSecret!,
                    verificationData.data as string,
                ))
            ) {
                throw apiError(401, 'errorsAuth.invalidTotp');
            }
        } else if (verificationData?.type === 'password') {
            if (currentUser.totpEnabled) {
                throw apiError(400, 'errorsAuth.passwordNotAllowed');
            }

            const passwordMatch =
                !!currentUser.password &&
                (await verify(currentUser.password, verificationData.data as string));

            if (!passwordMatch) {
                throw apiError(401, 'errorsAuth.invalidPassword');
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
                throw apiError(401, 'errorsAuth.passkeyNotFound');
            }

            const webauthnTicket = await storage.getItem<{
                expectedChallenge: string;
                expiresAt: number;
            }>(`webauthnTicket:${ticket}`);
            if (!webauthnTicket || webauthnTicket.expiresAt < Date.now()) {
                throw apiError(400, 'errorsAuth.invalidTicket');
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
                    transports: findCredentialById.transports,
                },
                requireUserVerification: true,
            });

            if (!response.verified) {
                throw apiError(401, 'errorsAuth.invalidPasskey');
            }

            await prisma.credential.update({
                where: {
                    id: findCredentialById.id,
                },
                data: {
                    counter: response.authenticationInfo.newCounter,
                },
            });
        } else if (verificationData?.type === 'oidc') {
            throw apiError(401, 'errorsAuth.oidcNotVerified');
        } else {
            throw apiError(400, 'errorsAuth.invalidVerificationType');
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
