export default defineNuxtRouteMiddleware(() => {
    const user = useAuthUser();
    if (!user.value) return navigateTo('/login');

    if (!user.value.superAdmin) {
        return abortNavigation({
            status: 403,
            message: 'You do not have permission to access this page',
        });
    }
});
