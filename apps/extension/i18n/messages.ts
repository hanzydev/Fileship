import uiAz from '@fileship/ui/i18n/locales/az.json';
import uiDe from '@fileship/ui/i18n/locales/de.json';
import uiEn from '@fileship/ui/i18n/locales/en.json';
import uiFr from '@fileship/ui/i18n/locales/fr.json';
import uiIt from '@fileship/ui/i18n/locales/it.json';
import uiJa from '@fileship/ui/i18n/locales/ja.json';
import uiKo from '@fileship/ui/i18n/locales/ko.json';
import uiTr from '@fileship/ui/i18n/locales/tr.json';
import uiZh from '@fileship/ui/i18n/locales/zh.json';

import az from './locales/az.json';
import de from './locales/de.json';
import en from './locales/en.json';
import fr from './locales/fr.json';
import it from './locales/it.json';
import ja from './locales/ja.json';
import ko from './locales/ko.json';
import tr from './locales/tr.json';
import zh from './locales/zh.json';

export const LANGUAGES = ['en', 'tr', 'de', 'fr', 'it', 'az', 'ja', 'ko', 'zh'] as const;

export type Language = (typeof LANGUAGES)[number];

export const messages = {
    en: { ...uiEn, ...en },
    tr: { ...uiTr, ...tr },
    de: { ...uiDe, ...de },
    fr: { ...uiFr, ...fr },
    it: { ...uiIt, ...it },
    az: { ...uiAz, ...az },
    ja: { ...uiJa, ...ja },
    ko: { ...uiKo, ...ko },
    zh: { ...uiZh, ...zh },
};
