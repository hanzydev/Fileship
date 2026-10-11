import { defu } from 'defu';
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
    if (!body.success) {
        throw createError({
            status: 400,
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    const reqUrl = getRequestURL(event);
    const storage = useStorage('cache');

    if (body.data.verify) {
        const credentialId = body.data.authenticationResponse?.id;
        const userHandle = body.data.authenticationResponse?.response?.userHandle;

        if (typeof credentialId !== 'string') {
            throw createError({
                status: 400,
                message: 'Invalid authentication response',
            });
        }

        const findCredentialById = await prisma.credential.findUnique({
            where: { id: credentialId },
            include: {
                user: {
                    include: {
                        _count: {
                            select: { files: true, folders: true, notes: true },
                        },
                    },
                },
            },
        });

        if (!findCredentialById) {
            throw createError({
                status: 404,
                message: "This passkey isn't registered here",
                data: { unknownCredential: true },
            });
        }

        if (
            userHandle &&
            Buffer.from(userHandle, 'base64url').toString() !== findCredentialById.userId
        ) {
            throw createError({
                status: 400,
                message: 'Invalid authentication response',
            });
        }

        const findUserById = findCredentialById.user;

        const webauthnTicket = await storage.getItem<{
            expectedChallenge: string;
            expiresAt: number;
        }>(`webauthnTicket:${body.data.ticket}`);
        if (!webauthnTicket || webauthnTicket.expiresAt < Date.now()) {
            throw createError({
                status: 400,
                message: 'Invalid or expired ticket',
            });
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

            const headers = getHeaders(event);
            const ip = getRequestIP(event, { xForwardedFor: true }) || 'Unknown';

            const { os, platform, location } = await getDevice(headers as never, ip);

            const sessionPrivateId = nanoid(128);

            const user = await prisma.user.update({
                where: {
                    id: findUserById.id,
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
                    context: { user: findUserById },
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

            sendToUser(findUserById.id, 'session:create', session);

            return {
                verified: true,
                user: {
                    id: findUserById.id,
                    username: findUserById.username,
                    avatar: findUserById.avatar,
                    permissions: findUserById.permissions,
                    createdAt: findUserById.createdAt,
                    totpEnabled: findUserById.totpEnabled,
                    superAdmin: findUserById.superAdmin,
                    aiSettings: defu(
                        findUserById.aiSettings,
                        defaultUserAiSettings,
                    ) as IUserAiSettings,
                    backupRestoreState: findUserById.backupRestoreState,
                    theme: findUserById.theme,
                    stats: findUserById._count,
                },
                session: {
                    id: session.id,
                    privateId: session.privateId,
                },
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
