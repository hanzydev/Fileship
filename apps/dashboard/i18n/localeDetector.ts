const LOCALES = ['en', 'tr'];

const pick = (value?: string | null) => {
    const language = value?.split('-')[0]?.toLowerCase();
    return language && LOCALES.includes(language) ? language : undefined;
};

export default defineI18nLocaleDetector((event, config) => {
    return (
        pick(event.context.user?.language) ||
        pick(tryCookieLocale(event, { lang: '', name: 'fileship_locale' })?.toString()) ||
        pick(tryHeaderLocale(event, { lang: '' })?.language) ||
        config.defaultLocale
    );
});
