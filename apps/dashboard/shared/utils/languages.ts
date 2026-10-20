export const LANGUAGES = ['en', 'tr', 'zh', 'de', 'fr', 'it'] as const;

export type Language = (typeof LANGUAGES)[number];
