import type { Theme } from '../../shared/ipc';

export default defineNuxtPlugin(async () => {
    if (['/overlay', '/recorder'].includes(window.location.pathname)) return;

    const account = useAccount();
    const updateStatus = useUpdateStatus();
    const savedSettings = useSettings();
    const colorMode = useColorMode();
    const router = useRouter();

    const [currentAccount, settings, status] = await Promise.all([
        desktop('account:get').catch(() => null),
        desktop('settings:get'),
        desktop('updates:get'),
    ]);

    account.value = currentAccount;
    savedSettings.value = settings;
    updateStatus.value = status;
    colorMode.preference = settings.theme;

    window.fileship.on('account:changed', (value) => {
        account.value = value;
        if (!value) router.push('/connect');
    });

    window.fileship.on('updates:status', (value) => {
        updateStatus.value = value;
    });

    window.fileship.on('app:navigate', (route) => router.push(route));

    watch(
        () => colorMode.preference,
        (theme) => {
            if (theme !== savedSettings.value?.theme) updateSettings({ theme: theme as Theme });
        },
    );

    watch(
        () => colorMode.value,
        (value) => desktop('window:theme', value === 'dark'),
        { immediate: true },
    );
});
