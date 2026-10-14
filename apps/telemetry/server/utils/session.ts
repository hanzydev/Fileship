import { createHash, randomBytes, timingSafeEqual } from 'node:crypto';

import type { H3Event } from 'h3';

const SESSION_MAX_AGE = 30 * 24 * 60 * 60;

const hash = (value: string) => createHash('sha256').update(value).digest();

export const hashToken = (token: string) => hash(token).toString('hex');

export const isAdminPassword = (password: string) => {
    const { adminPassword } = useRuntimeConfig();
    return !!adminPassword && timingSafeEqual(hash(password), hash(adminPassword));
};

export const findSession = async (token: string) => {
    const session = await prisma.session.findUnique({ where: { tokenHash: hashToken(token) } });
    return session && session.expiresAt > new Date() ? session : null;
};

export const startSession = async (event: H3Event) => {
    const token = randomBytes(32).toString('base64url');

    await prisma.session.create({
        data: {
            tokenHash: hashToken(token),
            expiresAt: new Date(Date.now() + SESSION_MAX_AGE * 1000),
        },
    });

    setCookie(event, 'token', token, {
        httpOnly: true,
        secure: getRequestProtocol(event, { xForwardedProto: true }) === 'https',
        sameSite: 'lax',
        path: '/',
        maxAge: SESSION_MAX_AGE,
    });
};

export const endSession = async (event: H3Event) => {
    const token = getCookie(event, 'token');
    if (token) await prisma.session.deleteMany({ where: { tokenHash: hashToken(token) } });

    deleteCookie(event, 'token');
};

export const adminOnly = (event: H3Event) => {
    if (!event.context.admin) {
        throw createError({
            status: 401,
            message: 'You need to log in',
        });
    }
};
