import uiAz from '@fileship/ui/i18n/locales/az.json';
import uiDe from '@fileship/ui/i18n/locales/de.json';
import uiEn from '@fileship/ui/i18n/locales/en.json';
import uiEs from '@fileship/ui/i18n/locales/es.json';
import uiFr from '@fileship/ui/i18n/locales/fr.json';
import uiIt from '@fileship/ui/i18n/locales/it.json';
import uiJa from '@fileship/ui/i18n/locales/ja.json';
import uiKo from '@fileship/ui/i18n/locales/ko.json';
import uiPt from '@fileship/ui/i18n/locales/pt.json';
import uiRu from '@fileship/ui/i18n/locales/ru.json';
import uiTr from '@fileship/ui/i18n/locales/tr.json';
import uiZh from '@fileship/ui/i18n/locales/zh.json';

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

export const messages = {
    en: { ...uiEn, ...en },
    tr: { ...uiTr, ...tr },
    de: { ...uiDe, ...de },
    es: { ...uiEs, ...es },
    fr: { ...uiFr, ...fr },
    it: { ...uiIt, ...it },
    pt: { ...uiPt, ...pt },
    ru: { ...uiRu, ...ru },
    az: { ...uiAz, ...az },
    ja: { ...uiJa, ...ja },
    ko: { ...uiKo, ...ko },
    zh: { ...uiZh, ...zh },
};
