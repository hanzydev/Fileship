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

export const toLanguage = (value?: string | null) => {
    const language = value?.split('-')[0]?.toLowerCase();
    return LANGUAGES.includes(language as Language) ? (language as Language) : undefined;
};
