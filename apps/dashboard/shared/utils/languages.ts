export const LANGUAGES = ['en', 'tr', 'zh', 'de', 'fr', 'it', 'az', 'ja', 'ko'] as const;

export type Language = (typeof LANGUAGES)[number];
