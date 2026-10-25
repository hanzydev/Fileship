import { type Language, messages } from '../i18n/messages';

type Params = Record<string, string | number>;

let language: Language = 'en';

const lookup = (locale: Language, key: string) =>
    key.split('.').reduce<any>((value, part) => value?.[part], messages[locale]);

const choose = (message: string, count?: number) => {
    const choices = message.split(' | ');
    if (count === undefined || choices.length === 1) return choices[0]!;
    if (choices.length === 2) return choices[Number(language === 'fr' ? count > 1 : count !== 1)]!;
    return choices[Math.min(count, 2)]!;
};

export const t = (key: string, params: Params = {}) => {
    const message = lookup(language, key) ?? lookup('en', key);
    if (typeof message !== 'string') return key;

    const count = typeof params.count === 'number' ? params.count : undefined;

    return choose(message, count)
        .replace(/\{'([^']*)'\}/g, '$1')
        .replace(/\{(\w+)\}/g, (match, name) => String(params[name] ?? match));
};

export const getLanguage = () => language;

export const setLanguage = (value: Language) => {
    language = value;
};
