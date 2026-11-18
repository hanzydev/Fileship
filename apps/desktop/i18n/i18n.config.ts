import { messagesFor } from './messages';

export default defineI18nConfig(() => ({
    fallbackLocale: 'en',
    missingWarn: false,
    fallbackWarn: false,
    messages: messagesFor(import.meta.client ? window.fileship.platform : 'win32'),
}));
