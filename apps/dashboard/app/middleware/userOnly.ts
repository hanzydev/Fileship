export default defineNuxtRouteMiddleware((to) => {
    const user = useAuthUser();
    if (!user.value)
        return navigateTo({
            path: '/login',
            query: { redirectTo: to.path },
        });
});
