import { pluralIndex } from '@fileship/ui/i18n/plural';

import { type Language, LANGUAGES, messages } from '../i18n/messages';

type Params = Record<string, string | number>;

let language: Language | undefined;

const resolveLanguage = (): Language => {
    const uiLanguage = browser.i18n.getUILanguage().split('-')[0]!.toLowerCase();
    return LANGUAGES.includes(uiLanguage as Language) ? (uiLanguage as Language) : 'en';
};

const lookup = (locale: Language, key: string) =>
    key.split('.').reduce<any>((value, part) => value?.[part], messages[locale]);

const choose = (message: string, count?: number) => {
    const choices = message.split(' | ');
    if (count === undefined || choices.length === 1) return choices[0]!;
    return choices[pluralIndex(getLanguage(), count, choices.length)]!;
};

export const getLanguage = () => (language ??= resolveLanguage());

export const t = (key: string, params: Params = {}) => {
    const message = lookup(getLanguage(), key) ?? lookup('en', key);
    if (typeof message !== 'string') return key;

    const count = typeof params.count === 'number' ? params.count : undefined;

    return choose(message, count)
        .replace(/\{'([^']*)'\}/g, '$1')
        .replace(/\{(\w+)\}/g, (match, name) => String(params[name] ?? match));
};
