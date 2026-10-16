export default defineNuxtRouteMiddleware(() => {
    const user = useAuthUser();
    if (!user.value) return navigateTo('/login');

    if (!isAdmin(user.value)) {
        return abortNavigation({
            status: 403,
            message: useNuxtApp().$i18n.t('auth.noPermission'),
        });
    }
});
