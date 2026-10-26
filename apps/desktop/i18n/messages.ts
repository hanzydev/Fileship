import az from './locales/az.json';
import de from './locales/de.json';
import en from './locales/en.json';
import es from './locales/es.json';
import fr from './locales/fr.json';
import it from './locales/it.json';
import ja from './locales/ja.json';
import ko from './locales/ko.json';
import pt from './locales/pt.json';
import ru from './locales/ru.json';
import tr from './locales/tr.json';
import zh from './locales/zh.json';

export const LANGUAGES = [
    'en',
    'tr',
    'de',
    'es',
    'fr',
    'it',
    'pt',
    'ru',
    'az',
    'ja',
    'ko',
    'zh',
] as const;

export type Language = (typeof LANGUAGES)[number];

export const messages = { en, tr, de, es, fr, it, pt, ru, az, ja, ko, zh };
