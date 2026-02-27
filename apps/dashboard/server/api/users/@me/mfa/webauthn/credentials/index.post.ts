import { nanoid } from 'nanoid';
import { z } from 'zod';

import { bufferToBase64URLString } from '@simplewebauthn/browser';
import { generateRegistrationOptions, verifyRegistrationResponse } from '@simplewebauthn/server';

const validationSchema = z.discriminatedUnion('verify', [
    z.object({
        verify: z.literal(true),
        ticket: z.string().length(32),
        registrationResponse: z.any(),
        name: z
            .string()
            .min(3, 'Name must be at least 3 characters')
            .max(32, 'Name must be less than 32 characters')
            .optional(),
        verificationData: z.any().optional(),
    }),
    z.object({
        verify: z.literal(false),
        verificationData: z.any().optional(),
    }),
]);

export default defineEventHandler(async (event) => {
    userOnly(event);

    const body = await readValidatedBody(event, validationSchema.safeParse);
    if (!body.success) {
        throw createError({
            statusCode: 400,
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    const currentUser = event.context.user!;

    const reqUrl = getRequestURL(event);
    const storage = useStorage('cache');

    await verifySession(event, body.data.verificationData);

    if (body.data.verify) {
        const webauthnTicket = await storage.getItem<{
            expectedChallenge: string;
            expiresAt: number;
        }>(`webauthnTicket:${body.data.ticket}`);
        if (!webauthnTicket || webauthnTicket.expiresAt < Date.now()) {
            throw createError({
                statusCode: 400,
                message: 'Invalid or expired ticket',
            });
        }

        const expectedChallenge = webauthnTicket.expectedChallenge;
        await storage.removeItem(`webauthnTicket:${body.data.ticket}`);

        const response = await verifyRegistrationResponse({
            response: body.data.registrationResponse,
            expectedChallenge,
            expectedOrigin: reqUrl.origin,
            expectedRPID: reqUrl.hostname,
            requireUserVerification: false,
        });

        if (response.verified) {
            const { registrationInfo } = response;

            const passkey = await prisma.credential.create({
                data: {
                    id: registrationInfo.credential.id,
                    name:
                        body.data.name ||
                        body.data.registrationResponse?.clientExtensionResults?.credProps
                            ?.authenticatorDisplayName,
                    publicKey: bufferToBase64URLString(
                        registrationInfo.credential.publicKey as never,
                    ),
                    counter: registrationInfo.credential.counter,
                    backedUp: registrationInfo.credentialBackedUp,
                    transports: registrationInfo.credential.transports,
                    userId: currentUser.id,
                },
                select: {
                    id: true,
                    name: true,
                    createdAt: true,
                },
            });

            sendToUser(currentUser.id, 'passkey:create', passkey);
        }

        return { verified: response.verified };
    }

    const ticket = nanoid(32);
    const registrationOptions = await generateRegistrationOptions({
        rpName: 'Fileship',
        rpID: reqUrl.hostname,
        userID: new TextEncoder().encode(currentUser.id),
        userName: currentUser.username,
        userDisplayName: currentUser.username,
        attestationType: 'none',
        authenticatorSelection: {
            userVerification: 'required',
        },
    });

    await storage.setItem(`webauthnTicket:${ticket}`, {
        expectedChallenge: registrationOptions.challenge,
        expiresAt: Date.now() + 5 * 60 * 1000, // 5 minutes
    });

    return {
        ticket,
        registrationOptions,
    };
});
