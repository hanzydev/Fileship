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
            htmlAttrs: {
                lang: 'en',
            },
        },
    },

    compatibilityDate: '2024-09-04',
});
