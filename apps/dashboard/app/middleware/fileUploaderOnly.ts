export default defineNuxtRouteMiddleware(() => {
    const user = useAuthUser();
    if (!user.value) return navigateTo('/login');

    if (!canUploadFiles(user)) {
        return abortNavigation({
            statusCode: 403,
            message: 'You do not have permission to access this page',
        });
    }
});
