import { createHash, randomBytes } from 'node:crypto';

import { defu } from 'defu';
import type { H3Event } from 'h3';

import type { User } from '~~/generated/prisma/client';

export const hashToken = (token: string) => createHash('sha256').update(token).digest('hex');

export const setTokenCookie = (event: H3Event, name: 'token' | 'adminToken', token: string) =>
    setCookie(event, name, token, {
        httpOnly: true,
        secure: getRequestProtocol(event, { xForwardedProto: true }) === 'https',
        sameSite: 'lax',
        path: '/',
        maxAge: 60 * 60 * 24 * 400,
    });

export const findSession = (token: string) =>
    prisma.session.findUnique({
        where: { tokenHash: hashToken(token) },
        include: {
            user: true,
            impersonator: { select: { id: true, user: { select: { username: true } } } },
        },
    });

export const getSessionUser = (session: NonNullable<Awaited<ReturnType<typeof findSession>>>) => ({
    ...session.user,
    currentSessionId: session.id,
    impersonator: session.impersonator && {
        sessionId: session.impersonator.id,
        username: session.impersonator.user.username,
    },
});

export const startSession = async (
    event: H3Event,
    user: User & { _count: { files: number; folders: number; notes: number } },
    log: { action: string; message: string },
    impersonatorSessionId?: string,
) => {
    const token = randomBytes(32).toString('base64url');
    const userAgent = getRequestHeader(event, 'user-agent') || '';
    const ip = getClientIp(event);
    const { os, platform } = getDevice(userAgent);

    const session = await prisma.session.create({
        data: {
            tokenHash: hashToken(token),
            userId: user.id,
            ip,
            os,
            platform,
            location: 'Unknown, Unknown, Unknown',
            userAgent,
            impersonatorSessionId,
        },
        select: { id: true, ip: true, os: true, platform: true, location: true, lastSeen: true },
    });

    event.waitUntil(
        getLocation(ip).then((location) =>
            prisma.session.update({ where: { id: session.id }, data: { location } }),
        ),
    );

    await createLog(Object.assign({}, event, { context: { user } }), {
        action: log.action,
        message: `${log.message} using ${os} on ${platform}`,
    });

    setTokenCookie(event, 'token', token);
    sendToUser(user.id, 'session:create', session);

    return {
        user: {
            id: user.id,
            username: user.username,
            avatar: user.avatar,
            permissions: user.permissions,
            createdAt: user.createdAt,
            totpEnabled: user.totpEnabled,
            superAdmin: user.superAdmin,
            aiSettings: defu(user.aiSettings, defaultUserAiSettings) as IUserAiSettings,
            backupRestoreState: user.backupRestoreState,
            theme: user.theme,
            stats: user._count,
        },
        session: { id: session.id },
    };
};
