export default defineEventHandler(async (event) => {
    event.context.admin = false;

    const token = getCookie(event, SESSION_COOKIE);
    if (!token) return;

    const session = await findSession(token);

    if (!session) {
        deleteCookie(event, SESSION_COOKIE);
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

    event.context.admin = true;
});
