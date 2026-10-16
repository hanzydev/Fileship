import vueI18n from '@intlify/eslint-plugin-vue-i18n';

const IGNORED_TEXT = [
    'Fileship',
    'ShareX',
    'Flameshot',
    'Windows',
    'S3',
    'SFTP',
    'FTP',
    'OCR',
    'GIF',
    'TOTP',
    'URL',
    'ID',
    'IP',
    'MB',
    'GB',
    'KB',
    'px',
    'fps',
    'ms',
    'English',
    'Türkçe',
];

export default (localeDir) => [
    {
        files: ['**/*.vue'],
        plugins: {
            '@intlify/vue-i18n': vueI18n,
        },
        settings: {
            'vue-i18n': {
                localeDir,
                messageSyntaxVersion: '^11.0.0',
            },
        },
        rules: {
            '@intlify/vue-i18n/no-raw-text': [
                'error',
                {
                    ignorePattern: String.raw`^[\s\d\-#:()&·•%+/|.,×—–…<>=*@!?\[\]{}"']+$`,
                    ignoreText: IGNORED_TEXT,
                    attributes: {
                        '/.+/': [
                            'title',
                            'description',
                            'label',
                            'placeholder',
                            'aria-label',
                            'alt',
                            'legend',
                            'confirm-text',
                            'value-label',
                        ],
                    },
                },
            ],
            '@intlify/vue-i18n/no-missing-keys': 'error',
        },
    },
];
