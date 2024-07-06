export default defineNuxtPlugin(async () => {
    const { data } = await useFetch('/api/users/@me');
    const currentUser = useAuthUser();

    if (data.value) {
        currentUser.value = {
            ...data.value,
            createdAt: new Date(data.value.createdAt),
        };
    }
});
