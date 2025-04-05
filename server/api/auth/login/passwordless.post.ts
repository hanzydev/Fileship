import { nanoid } from 'nanoid';
import { z } from 'zod';

import { base64URLStringToBuffer } from '@simplewebauthn/browser';
import {
    generateAuthenticationOptions,
    verifyAuthenticationResponse,
} from '@simplewebauthn/server';
import type { AuthenticatorTransportFuture } from '@simplewebauthn/types';

const validationSchema = z.object({
    verify: z.boolean({
        required_error: 'Missing verify',
        invalid_type_error: 'Invalid verify',
    }),
    expectedChallenge: z
        .string({
            invalid_type_error: 'Invalid expectedChallenge',
        })
        .optional(),
    authenticationResponse: z.any().optional(),
});

export default defineEventHandler(async (event) => {
    const body = await readValidatedBody(event, validationSchema.safeParse);
    if (!body.success) {
        throw createError({
            statusCode: 400,
            statusMessage: 'Bad Request',
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    const reqUrl = getRequestURL(event);

    if (body.data.verify) {
        const userHandle = body.data.authenticationResponse?.response?.userHandle;

        if (!userHandle) {
            throw createError({
                statusCode: 400,
                statusMessage: 'Bad Request',
                message: 'Invalid authentication response',
            });
        }

        const findUserByUsername = await prisma.user.findUnique({
            where: { id: Buffer.from(userHandle, 'base64url').toString() },
        });

        if (!findUserByUsername) {
            throw createError({
                statusCode: 404,
                statusMessage: 'Not Found',
                message: 'User not found',
            });
        }

        const findCredentialById = (await prisma.credential.findUnique({
            where: { id: body.data.authenticationResponse!.id, userId: findUserByUsername.id },
        }))!;

        if (!findCredentialById) {
            throw createError({
                statusCode: 404,
                statusMessage: 'Not Found',
                message: 'Credential not found',
            });
        }

        const response = await verifyAuthenticationResponse({
            response: body.data.authenticationResponse!,
            expectedChallenge: body.data.expectedChallenge!,
            expectedOrigin: reqUrl.origin,
            expectedRPID: reqUrl.hostname,
            requireUserVerification: true,
            credential: {
                id: findCredentialById.id,
                publicKey: new Uint8Array(base64URLStringToBuffer(findCredentialById.publicKey)),
                counter: findCredentialById.counter,
                transports: findCredentialById.transports as AuthenticatorTransportFuture[],
            },
        });

        if (response.verified) {
            await prisma.credential.update({
                where: { id: findCredentialById.id },
                data: {
                    counter: response.authenticationInfo.newCounter,
                },
            });

            const headers = getHeaders(event);
            const ip = getRequestIP(event, { xForwardedFor: true }) || 'Unknown';

            const { os, platform, location } = await getDevice(headers as never, ip);

            const sessionPrivateId = nanoid(128);

            const user = await prisma.user.update({
                where: {
                    id: findUserByUsername.id,
                },
                data: {
                    sessions: {
                        create: {
                            privateId: sessionPrivateId,
                            ip,
                            os,
                            platform,
                            location,
                        },
                    },
                },
                select: {
                    sessions: true,
                },
            });

            const session = user.sessions.find(
                (session) => session.privateId === sessionPrivateId,
            )!;

            await createLog(
                Object.assign({}, event, {
                    context: { user: findUserByUsername },
                }),
                {
                    action: 'Login',
                    message: `Logged in with passkey using ${os} on ${platform}`,
                },
            );

            setCookie(event, 'sessionId', sessionPrivateId, {
                expires: new Date(Date.now() + 1000 * 60 * 60 * 24 * 365),
                path: '/',
                sameSite: 'lax',
            });

            sendToUser(findUserByUsername.id, 'create:session', session);

            return {
                verified: true,
                user: {
                    id: findUserByUsername.id,
                    username: findUserByUsername.username,
                    avatar: findUserByUsername.avatar,
                    permissions: findUserByUsername.permissions,
                    createdAt: findUserByUsername.createdAt,
                    totpEnabled: findUserByUsername.totpEnabled,
                    superAdmin: findUserByUsername.superAdmin,
                    limits: findUserByUsername.limits,
                    backupRestoreState: findUserByUsername.backupRestoreState,
                    theme: findUserByUsername.theme,
                },
                session: {
                    id: session.id,
                    privateId: session.privateId,
                },
            };
        }

        return { verified: false };
    }

    return generateAuthenticationOptions({
        rpID: reqUrl.hostname,
        userVerification: 'required',
    });
});
