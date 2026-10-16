const UPLOAD_TOKEN_PATHS = ['/api/files', '/api/links'];

export default defineEventHandler(async (event) => {
    event.context.user = null;

    const cookieToken = getCookie(event, 'token');
    const bearerToken = cookieToken
        ? undefined
        : getRequestHeader(event, 'authorization')?.replace(/^Bearer /, '');

    const token = cookieToken || bearerToken;
    if (!token) return;

    const session = await findSession(token);

    if (!session) {
        if (cookieToken) {
            deleteCookie(event, 'token');
        } else if (event.method === 'POST' && UPLOAD_TOKEN_PATHS.includes(event.path)) {
            event.context.user = (await prisma.user.findUnique({
                where: { uploadTokenHash: hashToken(token) },
            })) as any;
        }

        return;
    }

    const fetchSite = getRequestHeader(event, 'sec-fetch-site');

    if (
        cookieToken &&
        !['GET', 'HEAD', 'OPTIONS'].includes(event.method) &&
        fetchSite &&
        !['same-origin', 'none'].includes(fetchSite)
    ) {
        throw apiError(403, 'errorsCommon.invalidOrigin');
    }

    event.context.user = getSessionUser(session) as any;
    if (event.context.user!.language) await refreshTranslation(event);

    if (Date.now() - session.lastSeen.getTime() > 60 * 60 * 1000) {
        if (cookieToken) setTokenCookie(event, 'token', token);

        event.waitUntil(
            prisma.session.update({ where: { id: session.id }, data: { lastSeen: new Date() } }),
        );
    }
});
