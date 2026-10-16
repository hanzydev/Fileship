import { messages } from './messages';

export default defineI18nConfig(() => ({
    fallbackLocale: 'en',
    missingWarn: false,
    fallbackWarn: false,
    messages,
}));
