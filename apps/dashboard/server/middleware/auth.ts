export default defineEventHandler(async (event) => {
    event.context.user = null;

    const token = getCookie(event, 'token');

    if (!token) {
        const authorization = getRequestHeader(event, 'authorization');

        if (
            authorization &&
            event.method === 'POST' &&
            ['/api/files', '/api/links'].includes(event.path)
        ) {
            event.context.user = (await prisma.user.findUnique({
                where: { uploadTokenHash: hashToken(authorization.replace(/^Bearer /, '')) },
            })) as any;
        }

        return;
    }

    const session = await findSession(token);

    if (!session) {
        deleteCookie(event, 'token');
        return;
    }

    const fetchSite = getRequestHeader(event, 'sec-fetch-site');

    if (
        !['GET', 'HEAD', 'OPTIONS'].includes(event.method) &&
        fetchSite &&
        !['same-origin', 'none'].includes(fetchSite)
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
