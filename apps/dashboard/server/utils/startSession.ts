import { defu } from 'defu';
import type { H3Event } from 'h3';
import { nanoid } from 'nanoid';

import type { User } from '~~/generated/prisma/client';

export const startSession = async (
    event: H3Event,
    user: User & { _count: { files: number; folders: number; notes: number } },
    log: { action: string; message: string },
) => {
    const headers = getHeaders(event);
    const ip = getRequestIP(event, { xForwardedFor: true }) || 'Unknown';

    const { os, platform, location } = await getDevice(headers as never, ip);

    const sessionPrivateId = nanoid(128);

    const { sessions } = await prisma.user.update({
        where: {
            id: user.id,
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

    const session = sessions.find((session) => session.privateId === sessionPrivateId)!;

    await createLog(Object.assign({}, event, { context: { user } }), {
        action: log.action,
        message: `${log.message} using ${os} on ${platform}`,
    });

    setCookie(event, 'sessionId', sessionPrivateId, {
        expires: new Date(Date.now() + 1000 * 60 * 60 * 24 * 365),
        path: '/',
        sameSite: 'lax',
    });

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
        session: {
            id: session.id,
            privateId: session.privateId,
        },
    };
};
