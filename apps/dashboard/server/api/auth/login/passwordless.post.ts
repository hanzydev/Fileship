import { nanoid } from 'nanoid';
import { z } from 'zod';

import { base64URLStringToBuffer } from '@simplewebauthn/browser';
import {
    generateAuthenticationOptions,
    verifyAuthenticationResponse,
} from '@simplewebauthn/server';

const validationSchema = z.union([
    z.object({
        verify: z.literal(true),
        ticket: z.string().length(32),
        authenticationResponse: z.any(),
    }),
    z.object({
        verify: z.literal(false),
    }),
]);

export default defineEventHandler(async (event) => {
    const body = await readValidatedBody(event, validationSchema.safeParse);
    if (!body.success) throw invalidBodyError(body.error);

    const reqUrl = getRequestURL(event);
    const storage = useStorage('cache');

    if (body.data.verify) {
        await rateLimit(`passkey:${getClientIp(event)}`, 10, 15 * 60 * 1000);

        const credentialId = body.data.authenticationResponse?.id;
        const userHandle = body.data.authenticationResponse?.response?.userHandle;

        if (typeof credentialId !== 'string') {
            throw apiError(400, 'errorsAuth.invalidAuthResponse');
        }

        const findCredentialById = await prisma.credential.findUnique({
            where: { id: credentialId },
            include: {
                user: {
                    include: {
                        _count: {
                            select: userCountSelect,
                        },
                    },
                },
            },
        });

        if (!findCredentialById) {
            throw apiError(404, 'errorsAuth.unknownPasskey', undefined, {
                unknownCredential: true,
            });
        }

        if (
            userHandle &&
            Buffer.from(userHandle, 'base64url').toString() !== findCredentialById.userId
        ) {
            throw apiError(400, 'errorsAuth.invalidAuthResponse');
        }

        const findUserById = findCredentialById.user;

        const webauthnTicket = await storage.getItem<{
            expectedChallenge: string;
            expiresAt: number;
        }>(`webauthnTicket:${body.data.ticket}`);
        if (!webauthnTicket || webauthnTicket.expiresAt < Date.now()) {
            throw apiError(400, 'errorsAuth.invalidTicket');
        }

        const expectedChallenge = webauthnTicket.expectedChallenge;
        await storage.removeItem(`webauthnTicket:${body.data.ticket}`);

        const response = await verifyAuthenticationResponse({
            response: body.data.authenticationResponse,
            expectedChallenge,
            expectedOrigin: reqUrl.origin,
            expectedRPID: reqUrl.hostname,
            requireUserVerification: true,
            credential: {
                id: findCredentialById.id,
                publicKey: new Uint8Array(base64URLStringToBuffer(findCredentialById.publicKey)),
                counter: findCredentialById.counter,
                transports: findCredentialById.transports,
            },
        });

        if (response.verified) {
            await prisma.credential.update({
                where: { id: findCredentialById.id },
                data: {
                    counter: response.authenticationInfo.newCounter,
                },
            });

            return {
                verified: true,
                ...(await startSession(event, findUserById, {
                    action: 'Login',
                    message: 'Logged in with passkey',
                })),
            };
        }

        return { verified: false };
    }

    const ticket = nanoid(32);
    const authenticationOptions = await generateAuthenticationOptions({
        rpID: reqUrl.hostname,
        userVerification: 'required',
    });

    await storage.setItem(`webauthnTicket:${ticket}`, {
        expectedChallenge: authenticationOptions.challenge,
        expiresAt: Date.now() + 5 * 60 * 1000, // 5 minutes
    });

    return {
        ticket,
        authenticationOptions,
    };
});
