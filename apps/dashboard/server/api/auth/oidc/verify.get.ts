export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;
    const settings = await getSettings();

    if (!currentUser) return sendRedirect(event, '/sso?status=expired');
    if (!isOidcConfigured(settings)) return sendRedirect(event, '/sso?status=unavailable');
    if (!isSiteHost(event, settings)) return sendRedirect(event, '/sso?status=wrongAddress');

    if (!(await findOidcIdentity(currentUser.id, settings))) {
        return sendRedirect(event, '/sso?status=wrongAccount');
    }

    const url = await rateLimit(`oidcVerify:${currentUser.id}`, 10, 15 * 60 * 1000)
        .then(() =>
            startOidcFlow(event, settings, {
                intent: 'verify',
                sessionId: currentUser.currentSessionId,
            }),
        )
        .catch((error) => (error?.status === 429 ? '/sso?status=tooManyAttempts' : null));

    return sendRedirect(event, url ?? '/sso?status=unavailable');
});
