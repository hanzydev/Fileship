export default defineNuxtRouteMiddleware((to) => {
    if (['/overlay', '/recorder', '/editor'].includes(to.path)) return;

    const account = useAccount();

    if (to.path === '/connect') {
        if (account.value) return navigateTo('/');
        return;
    }

    if (!account.value) return navigateTo('/connect');
});
