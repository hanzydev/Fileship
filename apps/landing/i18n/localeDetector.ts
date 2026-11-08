import { toLanguage } from '../shared/utils/languages';

export default defineI18nLocaleDetector((event, config) => {
    return (
        toLanguage(event.context.user?.language) ||
        toLanguage(tryCookieLocale(event, { lang: '', name: 'fileship_locale' })?.toString()) ||
        toLanguage(tryHeaderLocale(event, { lang: '' })?.language) ||
        config.defaultLocale
    );
});
