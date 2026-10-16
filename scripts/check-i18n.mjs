import { readdirSync, readFileSync } from 'node:fs';
import { basename, join } from 'node:path';

import { baseCompile } from '@intlify/message-compiler';

const LOCALES = ['en', 'tr'];

const catalogs = [
    { name: 'ui', files: (locale) => [`packages/ui/i18n/locales/${locale}.json`] },
    ...['apps/dashboard', 'apps/desktop'].map((app) => ({
        name: app,
        namespaced: true,
        files: (locale) =>
            readdirSync(join(app, 'i18n/locales', locale)).map((file) =>
                join(app, 'i18n/locales', locale, file),
            ),
    })),
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

const load = (catalog, locale) => {
    const messages = new Map();

    for (const file of catalog.files(locale)) {
        let json;
        try {
            json = JSON.parse(readFileSync(file, 'utf8'));
        } catch (error) {
            errors.push(`${file}: invalid JSON (${error.message})`);
            continue;
        }

        const namespace = basename(file, '.json');
        const roots = Object.keys(json);
        if (catalog.namespaced && roots.some((root) => root !== namespace))
            errors.push(`${file}: the only root key must be "${namespace}"`);

        for (const [key, value] of flatten(json, '', new Map())) {
            if (messages.has(key)) errors.push(`${file}: duplicate key ${key}`);
            if (typeof value !== 'string' || !value.trim())
                errors.push(`${file}: ${key} must be a non-empty string`);
            else
                baseCompile(value, {
                    onError: (error) => errors.push(`${file}: ${key}: ${error.message}`),
                });
            messages.set(key, value);
        }
    }

    return messages;
};

for (const catalog of catalogs) {
    const [base, ...others] = LOCALES.map((locale) => load(catalog, locale));

    others.forEach((messages, index) => {
        const locale = LOCALES[index + 1];
        for (const key of base.keys())
            if (!messages.has(key)) errors.push(`${catalog.name}: ${locale} is missing ${key}`);
        for (const key of messages.keys())
            if (!base.has(key)) errors.push(`${catalog.name}: ${locale} has extra key ${key}`);
    });
}

if (errors.length) {
    console.error(errors.join('\n'));
    console.error(`\n${errors.length} i18n problem(s)`);
    process.exit(1);
}

console.log('i18n catalogs are complete');
