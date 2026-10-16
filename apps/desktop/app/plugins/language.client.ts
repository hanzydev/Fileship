export default defineNuxtPlugin(async () => {
    const { $i18n } = useNuxtApp();

    await $i18n.setLocale(await desktop('app:language'));

    window.fileship.on('app:language', (language) => $i18n.setLocale(language));
});
