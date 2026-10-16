export default defineNuxtRouteMiddleware(() => {
    const user = useAuthUser();
    if (!user.value) return navigateTo('/login');

    if (!canShortenLinks(user)) {
        return abortNavigation({
            status: 403,
            message: useNuxtApp().$i18n.t('auth.noPermission'),
        });
    }
});
