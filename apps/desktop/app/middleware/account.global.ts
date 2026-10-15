export default defineNuxtRouteMiddleware((to) => {
    if (to.path === '/overlay') return;

    const account = useAccount();

    if (to.path === '/connect') {
        if (account.value) return navigateTo('/');
        return;
    }

    if (!account.value) return navigateTo('/connect');
});
