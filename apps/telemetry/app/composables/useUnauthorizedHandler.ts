export const useUnauthorizedHandler = () => {
    const nuxtApp = useNuxtApp();
    const admin = useAdmin();
    const route = useRoute();

    return ({ response }: { response: Response }) => {
        if (response.status !== 401) return;

        admin.value = false;
        nuxtApp.runWithContext(() =>
            navigateTo({ path: '/login', query: { redirectTo: route.fullPath } }),
        );
    };
};
