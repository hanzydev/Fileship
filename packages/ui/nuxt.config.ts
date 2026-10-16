import tailwindcss from '@tailwindcss/vite';

export default defineNuxtConfig({
    $meta: {
        name: 'ui',
    },

    future: {
        compatibilityVersion: 5,
    },

    modules: [
        '@nuxt/eslint',
        '@nuxt/fonts',
        '@vueuse/nuxt',
        '@nuxtjs/color-mode',
        'vue-sonner/nuxt',
        'nuxt-lucide-icons',
        '@nuxtjs/i18n',
    ],

    components: [
        {
            path: 'components',
            extensions: ['.vue'],
        },
    ],

    fonts: {
        families: [
            {
                name: 'Geist',
                provider: 'google',
                weights: [400, 500, 600, 700, 800],
            },
        ],
    },

    colorMode: {
        classSuffix: '',
        preference: 'system',
        fallback: 'dark',
        storage: 'cookie',
    },

    i18n: {
        strategy: 'no_prefix',
        defaultLocale: 'en',
        locales: [{ code: 'en', language: 'en-US', name: 'English' }],
        detectBrowserLanguage: false,
    },

    lucide: {
        namePrefix: 'I',
    },

    vite: {
        plugins: [tailwindcss()],
        optimizeDeps: {
            include: [
                '@fileship/ui > @lucide/vue',
                '@fileship/ui > @unovis/ts',
                '@fileship/ui > @unovis/vue',
                '@fileship/ui > class-variance-authority',
                '@fileship/ui > clsx',
                '@fileship/ui > reka-ui',
                '@fileship/ui > tailwind-merge',
                '@fileship/ui > vue-input-otp',
            ],
        },
    },

    compatibilityDate: '2024-09-04',
});
