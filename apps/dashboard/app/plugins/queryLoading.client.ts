export default defineNuxtPlugin(() => {
    const router = useRouter();
    const { finish } = useLoadingIndicator();

    router.afterEach((to, from) => {
        if (to.path === from.path) finish({ force: true });
    });
});
