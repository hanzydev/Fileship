export const useImpersonation = () => {
    const currentUser = useAuthUser();
    const currentTheme = useTheme();
    const adminSessionId = useCookie('adminSessionId', {
        maxAge: 60 * 60 * 6,
        path: '/',
        sameSite: 'lax',
    });

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
        await refreshNuxtData();
        await navigateTo('/dashboard');

        initSocket();
    };

    const stopImpersonating = async () => {
        closeSocket();

        await $fetch('/api/auth/logout', { method: 'POST' });

        const user = await $fetch<AuthUserData>('/api/users/@me', {
            headers: { Authorization: adminSessionId.value! },
        });

        currentUser.value = { ...user, createdAt: new Date(user.createdAt) };
        currentTheme.value = user.theme;

        useCookie('sessionId', {
            maxAge: 60 * 60 * 24 * 365,
            path: '/',
            sameSite: 'lax',
        }).value = adminSessionId.value;

        await nextTick();

        adminSessionId.value = null;

        await refreshNuxtData();
        await navigateTo('/dashboard/users');

        clearStates(true);
        initSocket();
    };

    return { adminSessionId, impersonate, stopImpersonating };
};
