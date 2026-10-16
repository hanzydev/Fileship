import en from './locales/en.json';
import tr from './locales/tr.json';

export const LANGUAGES = ['en', 'tr'] as const;

export type Language = (typeof LANGUAGES)[number];

export const messages = { en, tr };
