export default defineEventHandler(async (event) => {
    event.context.user = null;

    const token = getCookie(event, 'token');
    if (!token) return;

    const session = await findSession(token);

    if (!session) {
        deleteCookie(event, 'token');
        return;
    }

    const origin = getRequestHeader(event, 'origin');

    if (
        !['GET', 'HEAD', 'OPTIONS'].includes(event.method) &&
        origin &&
        origin.replace(/^https?:\/\//, '') !== getRequestHost(event, { xForwardedHost: true })
    ) {
        throw createError({
            status: 403,
            message: 'Invalid origin',
        });
    }

    event.context.user = getSessionUser(session) as any;

    if (Date.now() - session.lastSeen.getTime() > 60 * 60 * 1000) {
        setTokenCookie(event, 'token', token);
        event.waitUntil(
            prisma.session.update({ where: { id: session.id }, data: { lastSeen: new Date() } }),
        );
    }
});
