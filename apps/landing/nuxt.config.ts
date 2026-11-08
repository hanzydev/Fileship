export default defineNuxtConfig({
    extends: ['@fileship/ui'],

    future: {
        compatibilityVersion: 5,
    },

    modules: ['@nuxt/content', '@nuxtjs/i18n', '@vueuse/nuxt/module', '@nuxt/eslint', '@nuxt/image'],

    devtools: { enabled: true },

    devServer: {
        host: 'localhost',
    },

    compatibilityDate: '2024-04-03',

    components: [
        {
            path: '~/components',
            extensions: ['.vue'],
        },
    ],

    css: ['~/styles/tailwind.css','highlight.js/styles/tokyo-night-dark.css'],

    i18n: {
        locales: [
            { code: 'en', language: 'en-US', name: 'English', file: 'en.json' },
            { code: 'tr', language: 'tr-TR', name: 'Türkçe', file: 'tr.json' },
            { code: 'de', language: 'de-DE', name: 'Deutsch', file: 'de.json' },
            { code: 'es', language: 'es', name: 'Español', file: 'es.json' },
            { code: 'fr', language: 'fr-FR', name: 'Français', file: 'fr.json' },
            { code: 'it', language: 'it-IT', name: 'Italiano', file: 'it.json' },
            { code: 'pt', language: 'pt-BR', name: 'Português (Brasil)', file: 'pt.json' },
            { code: 'ru', language: 'ru-RU', name: 'Русский', file: 'ru.json' },
            { code: 'az', language: 'az-AZ', name: 'Azərbaycanca', file: 'az.json' },
            { code: 'ja', language: 'ja-JP', name: '日本語', file: 'ja.json' },
            { code: 'ko', language: 'ko-KR', name: '한국어', file: 'ko.json' },
            { code: 'zh', language: 'zh-CN', name: '简体中文', file: 'zh.json' },
        ],
        detectBrowserLanguage: {
            useCookie: true,
            cookieKey: 'fileship_locale',
            fallbackLocale: 'en',
        },
        experimental: {
            localeDetector: 'localeDetector.ts',
        },
    },

    imports: {
        autoImport: true,
    },

    experimental: {
        nitroAutoImports: true,
    },

    nitro: {
        esbuild: {
            options: {
                target: 'esnext',
            },
        },
    },

    app: {
        buildAssetsDir: '/_hanzy_was_here/',
        rootAttrs: {
            id: '__fileship_landing',
        },
        head: {
            title: 'Fileship',
            link: [
                {
                    rel: 'icon',
                    href: '/favicon.ico',
                },
                {
                    rel: 'apple-touch-icon',
                    href: '/apple-touch-icon.png',
                },
            ],
        },
    },

    vite: {
        build: {
            rollupOptions: {
                output: {
                    banner: `/*
* Fileship Landing
* https://github.com/hanzydev/Fileship
* (c) 2024-present Hànzy and Fileship contributors
* @license MIT
*/`,
                },
            },
        },
        optimizeDeps: {
            include: [
                'dayjs', // CJS
                'dayjs/locale/de', // CJS
                'dayjs/locale/es', // CJS
                'dayjs/locale/fr', // CJS
                'dayjs/locale/it', // CJS
                'dayjs/locale/pt-br', // CJS
                'dayjs/locale/ru', // CJS
                'dayjs/locale/az', // CJS
                'dayjs/locale/ja', // CJS
                'dayjs/locale/ko', // CJS
                'dayjs/locale/tr', // CJS
                'dayjs/locale/zh', // CJS
                'dayjs/plugin/duration', // CJS
                'dayjs/plugin/localizedFormat', // CJS
                'dayjs/plugin/relativeTime', // CJS
                'highlight.js',
            ],
        },
    },
});
