export default defineNuxtRouteMiddleware((to) => {
    const admin = useAdmin();

    if (to.path === '/login') {
        if (admin.value) return navigateTo('/overview');
        return;
    }

    if (!admin.value) {
        return navigateTo({
            path: '/login',
            query: { redirectTo: to.fullPath },
        });
    }
});
