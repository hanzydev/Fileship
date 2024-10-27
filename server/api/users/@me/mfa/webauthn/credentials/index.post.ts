import { z } from 'zod';

import { bufferToBase64URLString } from '@simplewebauthn/browser';
import {
    generateRegistrationOptions,
    verifyRegistrationResponse,
} from '@simplewebauthn/server';

const validationSchema = z.object({
    name: z
        .string({ invalid_type_error: 'Invalid name' })
        .min(3, { message: 'Name must be at least 3 characters' })
        .max(32, { message: 'Name must be less than 32 characters' })
        .optional(),
    verify: z.boolean({
        required_error: 'Missing verify',
        invalid_type_error: 'Invalid verify',
    }),
    expectedChallenge: z
        .string({
            invalid_type_error: 'Invalid expectedChallenge',
        })
        .optional(),
    registrationResponse: z.any().optional(),
    verificationData: z.any().optional(),
});

export default defineEventHandler(async (event) => {
    userOnly(event);

    const body = await readValidatedBody(event, validationSchema.safeParse);
    if (!body.success) {
        throw createError({
            statusCode: 400,
            statusMessage: 'Bad Request',
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    const currentUser = event.context.user!;
    const reqUrl = getRequestURL(event);

    await verifySession(event, body.data.verificationData);

    if (body.data.verify) {
        const response = await verifyRegistrationResponse({
            response: body.data.registrationResponse!,
            expectedChallenge: body.data.expectedChallenge!,
            expectedOrigin: reqUrl.origin,
            expectedRPID: reqUrl.hostname,
            requireUserVerification: false,
        });

        if (response.verified) {
            const { registrationInfo } = response;

            const id = registrationInfo!.credential.id;
            const name =
                body.data.name ||
                body.data.registrationResponse!.clientExtensionResults
                    ?.credProps?.authenticatorDisplayName ||
                'Untitled';

            await prisma.user.update({
                where: { id: currentUser.id },
                data: {
                    credentials: {
                        create: {
                            id,
                            name,
                            publicKey: bufferToBase64URLString(
                                registrationInfo!.credential.publicKey as never,
                            ),
                            counter: registrationInfo!.credential.counter,
                            backedUp: registrationInfo!.credentialBackedUp,
                            transports: registrationInfo!.credential.transports,
                        },
                    },
                },
            });

            sendToUser(currentUser.id, 'create:passkey', {
                id,
                name,
                createdAt: new Date(),
            });
        }

        return { verified: response.verified };
    }

    return generateRegistrationOptions({
        rpName: 'Fileship',
        rpID: reqUrl.hostname,
        userID: new TextEncoder().encode(currentUser.id),
        userName: currentUser.username,
        userDisplayName: currentUser.username,
        attestationType: 'none',
        authenticatorSelection: {
            userVerification: 'preferred',
        },
    });
});
