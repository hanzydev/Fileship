import de from './locales/de.json';
import en from './locales/en.json';
import fr from './locales/fr.json';
import it from './locales/it.json';
import tr from './locales/tr.json';
import zh from './locales/zh.json';

export const LANGUAGES = ['en', 'tr', 'zh', 'de', 'fr', 'it'] as const;

export type Language = (typeof LANGUAGES)[number];

export const messages = { en, tr, zh, de, fr, it };
