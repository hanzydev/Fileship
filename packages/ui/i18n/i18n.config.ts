import en from './locales/en.json';
import tr from './locales/tr.json';

export default defineI18nConfig(() => ({
    fallbackLocale: 'en',
    missingWarn: false,
    fallbackWarn: false,
    messages: { en, tr },
}));
