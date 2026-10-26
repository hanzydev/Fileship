export default defineNuxtConfig({
    extends: ['@fileship/ui'],

    future: {
        compatibilityVersion: 5,
    },

    ssr: false,

    components: [
        {
            path: '~/components',
            extensions: ['.vue'],
        },
    ],

    css: ['~/styles/tailwind.css'],

    i18n: {
        locales: [
            { code: 'en', language: 'en-US', name: 'English' },
            { code: 'tr', language: 'tr-TR', name: 'Türkçe' },
            { code: 'de', language: 'de-DE', name: 'Deutsch' },
            { code: 'es', language: 'es', name: 'Español' },
            { code: 'fr', language: 'fr-FR', name: 'Français' },
            { code: 'it', language: 'it-IT', name: 'Italiano' },
            { code: 'pt', language: 'pt-BR', name: 'Português (Brasil)' },
            { code: 'ru', language: 'ru-RU', name: 'Русский' },
            { code: 'az', language: 'az-AZ', name: 'Azərbaycanca' },
            { code: 'ja', language: 'ja-JP', name: '日本語' },
            { code: 'ko', language: 'ko-KR', name: '한국어' },
            { code: 'zh', language: 'zh-CN', name: '简体中文' },
        ],
    },

    colorMode: {
        storage: 'localStorage',
    },

    imports: {
        autoImport: true,
    },

    experimental: {
        nitroAutoImports: true,
    },

    devServer: {
        port: 3020,
    },

    app: {
        buildAssetsDir: '/_hanzy_was_here/',
        rootAttrs: {
            id: '__fileship_desktop',
        },
        head: {
            title: 'Fileship',
        },
    },

    compatibilityDate: '2024-09-04',
});
