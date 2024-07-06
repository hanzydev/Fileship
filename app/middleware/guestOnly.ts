export default defineNuxtRouteMiddleware(() => {
    const user = useAuthUser();
    if (user.value) return navigateTo('/dashboard');
});
