export const useImpersonation = () => {
    const embed = useEmbed();
    const domains = useDomains();
    const currentUser = useAuthUser();
    const currentTheme = useTheme();
    const adminSessionId = useCookie('adminSessionId', {
        maxAge: 60 * 60 * 6,
        path: '/',
        sameSite: 'lax',
    });

    const loadAccountData = async () => {
        [embed.value, domains.value] = await Promise.all([
            $fetch<IEmbed>('/api/users/@me/embed'),
            $fetch<string[]>('/api/users/@me/domains'),
        ]);
    };

    const impersonate = async (username: string) => {
        const adminSession = useCookie('sessionId').value;

        const result = await withVerification((verificationData) =>
            $fetch('/api/auth/login', {
                method: 'POST',
                body: { username, password: '--------', verificationData },
            }),
        );

        if (!result) return;

        const { user, session } = result as any;

        adminSessionId.value = adminSession;

        currentUser.value = {
            ...user,
            currentSessionId: session.id,
            createdAt: new Date(user.createdAt),
        };
        currentTheme.value = user.theme;

        clearStates(true);

        await nextTick();
        await loadAccountData();
        await refreshNuxtData();
        await navigateTo('/dashboard');

        initSocket();
    };

    const stopImpersonating = async () => {
        closeSocket();

        await $fetch('/api/auth/logout', { method: 'POST' }).catch(() => null);

        const user = await $fetch<AuthUserData>('/api/users/@me', {
            headers: { Authorization: adminSessionId.value! },
        }).catch(() => null);

        if (!user) {
            adminSessionId.value = null;
            useCookie('sessionId', { path: '/', sameSite: 'lax' }).value = null;
            clearStates(true, true);
            await navigateTo('/login');
            return;
        }

        currentUser.value = { ...user, createdAt: new Date(user.createdAt) };
        currentTheme.value = user.theme;

        useCookie('sessionId', {
            maxAge: 60 * 60 * 24 * 365,
            path: '/',
            sameSite: 'lax',
        }).value = adminSessionId.value;

        await nextTick();

        adminSessionId.value = null;

        clearStates(true);

        await loadAccountData();
        await refreshNuxtData();
        await navigateTo('/dashboard/users');

        initSocket();
    };

    return { adminSessionId, impersonate, stopImpersonating };
};
