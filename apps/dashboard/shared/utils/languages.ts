export const LANGUAGES = ['en', 'tr', 'de', 'fr', 'it', 'az', 'ja', 'ko', 'zh'] as const;

export type Language = (typeof LANGUAGES)[number];
