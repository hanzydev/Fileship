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
        ],
    },

    colorMode: {
        storage: 'localStorage',
    },

    imports: {
        autoImport: true,
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
