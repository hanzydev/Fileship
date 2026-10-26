import az from './locales/az.json';
import de from './locales/de.json';
import en from './locales/en.json';
import fr from './locales/fr.json';
import it from './locales/it.json';
import ja from './locales/ja.json';
import ko from './locales/ko.json';
import tr from './locales/tr.json';
import zh from './locales/zh.json';
import { pluralIndex } from './plural';

export default defineI18nConfig(() => ({
    fallbackLocale: 'en',
    missingWarn: false,
    fallbackWarn: false,
    messages: { en, tr, zh, de, fr, it, az, ja, ko },
    pluralRules: {
        fr: (choice: number, choicesLength: number) => pluralIndex('fr', choice, choicesLength),
    },
}));
