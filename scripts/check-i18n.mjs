import { readFileSync } from 'node:fs';

import { baseCompile } from '@intlify/message-compiler';

const LOCALES = ['en', 'tr', 'de', 'fr', 'it', 'az', 'ja', 'ko', 'zh'];
const CATALOGS = [
    'packages/ui/i18n/locales',
    'apps/dashboard/i18n/locales',
    'apps/desktop/i18n/locales',
];

const errors = [];

const flatten = (value, prefix, out) => {
    for (const [key, child] of Object.entries(value)) {
        const path = prefix ? `${prefix}.${key}` : key;
        if (child && typeof child === 'object') flatten(child, path, out);
        else out.set(path, child);
    }
    return out;
};

const load = (file) => {
    const messages = flatten(JSON.parse(readFileSync(file, 'utf8')), '', new Map());

    for (const [key, value] of messages) {
        if (typeof value !== 'string' || !value.trim())
            errors.push(`${file}: ${key} must be a non-empty string`);
        else
            baseCompile(value, {
                onError: (error) => errors.push(`${file}: ${key}: ${error.message}`),
            });
    }

    return messages;
};

for (const catalog of CATALOGS) {
    const [base, ...others] = LOCALES.map((locale) => load(`${catalog}/${locale}.json`));

    others.forEach((messages, index) => {
        const locale = LOCALES[index + 1];
        for (const key of base.keys())
            if (!messages.has(key)) errors.push(`${catalog}: ${locale} is missing ${key}`);
        for (const key of messages.keys())
            if (!base.has(key)) errors.push(`${catalog}: ${locale} has extra key ${key}`);
    });
}

if (errors.length) {
    console.error(errors.join('\n'));
    console.error(`\n${errors.length} i18n problem(s)`);
    process.exit(1);
}

console.log('i18n catalogs are complete');
