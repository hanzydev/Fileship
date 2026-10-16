export const useImpersonation = () => {
    const embed = useEmbed();
    const domains = useDomains();
    const currentUser = useAuthUser();
    const currentTheme = useTheme();
    const { $toast, $i18n } = useNuxtApp();

    const loadAccountData = async () => {
        [embed.value, domains.value] = await Promise.all([
            $fetch<IEmbed>('/api/users/@me/embed'),
            $fetch<string[]>('/api/users/@me/domains'),
        ]);
    };

    const impersonate = async (username: string) => {
        const result = await withVerification((verificationData) =>
            $fetch('/api/auth/impersonate', {
                method: 'POST',
                body: { username, verificationData },
            }),
        );

        if (!result) return;

        const { user, session } = result as any;
        const id = $toast.loading($i18n.t('actions.signingInAs', { username: user.username }));

        currentUser.value = {
            ...user,
            currentSessionId: session.id,
            impersonator: currentUser.value!.username,
            createdAt: new Date(user.createdAt),
        };
        currentTheme.value = user.theme;
        await applyLanguage(user.language);

        clearStates(true);

        try {
            await nextTick();
            await loadAccountData();
            await refreshNuxtData();
            await navigateTo('/dashboard');
        } finally {
            $toast.dismiss(id);
        }

        initSocket();
    };

    const stopImpersonating = async () => {
        closeSocket();

        await $fetch('/api/auth/impersonate/stop', { method: 'POST' });

        const user = await $fetch<AuthUserData>('/api/users/@me');

        currentUser.value = { ...user, createdAt: new Date(user.createdAt) };
        currentTheme.value = user.theme;
        await applyLanguage(user.language);

        clearStates(true);

        await loadAccountData();
        await refreshNuxtData();
        await navigateTo('/dashboard/users');

        initSocket();
    };

    return { impersonate, stopImpersonating };
};
