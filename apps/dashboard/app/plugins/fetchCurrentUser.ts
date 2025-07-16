export default defineNuxtPlugin(async () => {
    const { data } = await useFetch<AuthUserData>('/api/users/@me');

    const currentUser = useAuthUser();
    const currentTheme = useTheme();

    if (data.value) {
        currentUser.value = {
            ...data.value,
            createdAt: new Date(data.value.createdAt),
        };

        currentTheme.value = data.value.theme as never;
    }
});
