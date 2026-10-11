export default defineNuxtPlugin(async () => {
    const { data } = await useFetch('/api/settings/public');

    if (data.value) usePublicSettings().value = data.value;
});
