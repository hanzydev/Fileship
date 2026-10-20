import { type Language, LANGUAGES } from '../shared/utils/languages';

const pick = (value?: string | null) => {
    const language = value?.split('-')[0]?.toLowerCase();
    return language && LANGUAGES.includes(language as Language) ? language : undefined;
};

export default defineI18nLocaleDetector((event, config) => {
    return (
        pick(event.context.user?.language) ||
        pick(tryCookieLocale(event, { lang: '', name: 'fileship_locale' })?.toString()) ||
        pick(tryHeaderLocale(event, { lang: '' })?.language) ||
        config.defaultLocale
    );
});
