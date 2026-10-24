import { defineConfig } from 'eslint/config';
import vue from 'eslint-plugin-vue';
import tseslint from 'typescript-eslint';
import vueParser from 'vue-eslint-parser';

import base from '@fileship/eslint-config';
import i18n from '@fileship/eslint-config/i18n';
import nuxt from '@fileship/eslint-config/nuxt';

export default defineConfig(
    vue.configs['flat/recommended'],
    base,
    {
        files: ['**/*.vue'],
        languageOptions: {
            parser: vueParser,
            parserOptions: {
                parser: tseslint.parser,
                extraFileExtensions: ['.vue'],
                sourceType: 'module',
            },
        },
    },
    {
        files: ['**/*.ts', '**/*.vue'],
        rules: { 'no-undef': 'off' },
    },
    nuxt,
    i18n([
        { pattern: './i18n/locales/*.json', localeKey: 'file' },
        { pattern: '../../packages/ui/i18n/locales/*.json', localeKey: 'file' },
    ]),
    { ignores: ['.output/**', '.wxt/**'] },
);
