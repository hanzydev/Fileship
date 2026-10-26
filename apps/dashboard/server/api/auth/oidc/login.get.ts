export default defineEventHandler(async (event) => {
    const settings = await getSettings();
    const query = getQuery(event);
    const redirectTo =
        typeof query.redirectTo === 'string' && /^\/(?![/\\])/.test(query.redirectTo)
            ? query.redirectTo
            : undefined;

    if (!isOidcConfigured(settings)) return sendRedirect(event, '/login?sso=unavailable');

    if (!isSiteHost(event, settings) && !query.hop) {
        const params = new URLSearchParams({ ...(redirectTo && { redirectTo }), hop: '1' });
        return sendRedirect(
            event,
            new URL(`/api/auth/oidc/login?${params}`, settings.siteUrl).href,
        );
    }

    const url = await rateLimit(`oidcLogin:${getClientIp(event)}`, 30, 15 * 60 * 1000)
        .then(() => startOidcFlow(event, settings, { intent: 'login', redirectTo }))
        .catch((error) => (error?.status === 429 ? '/login?sso=tooManyAttempts' : null));

    return sendRedirect(event, url ?? '/login?sso=unavailable');
});
