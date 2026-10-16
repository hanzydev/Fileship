export default defineNuxtPlugin(async () => {
    const { data } = await useFetch('/api/auth/session');

    useAdmin().value = !!data.value?.admin;
});
