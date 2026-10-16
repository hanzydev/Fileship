import i18n from '@fileship/eslint-config/i18n';
import nuxt from '@fileship/eslint-config/nuxt';

import withNuxt from './.nuxt/eslint.config.mjs';

export default withNuxt(
    nuxt,
    i18n([
        {
            pattern: './i18n/locales/*/*.json',
            localeKey: 'path',
            localePattern: /^.*\/(?<locale>[a-z]+)\/[^/]+\.json$/,
        },
        { pattern: '../../packages/ui/i18n/locales/*.json', localeKey: 'file' },
    ]),
);
