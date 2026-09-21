import tailwindcss from '@tailwindcss/vite';

import pkg from '../../package.json';

const corsHeaders = {
    'Access-Control-Allow-Origin': '*',
    'Access-Control-Allow-Methods': '*',
    'Access-Control-Allow-Credentials': 'true',
    'Access-Control-Allow-Headers': '*',
    'Access-Control-Expose-Headers': '*',
};

export default defineNuxtConfig({
    future: {
        compatibilityVersion: 5,
    },

    modules: [
        '@nuxtjs/turnstile',
        '@nuxt/eslint',
        '@nuxt/fonts',
        '@vueuse/nuxt',
        '@nuxt/icon',
        'vue-sonner/nuxt',
        'nuxt-lucide-icons',
    ].concat((process.env.ENABLE_PWA || 'true') === 'true' ? ['@vite-pwa/nuxt'] : []),

    css: [
        '~/styles/tailwind.css',
        'katex/dist/katex.min.css',
        'highlight.js/styles/tokyo-night-dark.css',
    ],

    fonts: {
        families: [
            {
                name: 'Nunito Sans',
                provider: 'google',
                weights: [300, 400, 500, 600, 700],
            },
        ],
    },

    lucide: {
        namePrefix: 'I',
    },

    imports: {
        autoImport: true,
    },

    devtools: {
        enabled: true,
    },

    experimental: {
        clientNodeCompat: true,
        nitroAutoImports: true,
    },

    nitro: {
        experimental: {
            websocket: true,
            tasks: true,
        },
        esbuild: {
            options: {
                target: 'esnext',
            },
        },
        scheduledTasks: {
            [process.env.DELETE_EXPIRED_CRON || '*/5 * * * *']: 'db:deleteExpired',
            [process.env.GENERATE_STATS_CRON || '*/30 * * * *']: 'db:generateStats',

            '0 */2 * * *': 'telemetry:collect',
        },
        framework: {
            name: pkg.name,
            version: pkg.version,
        },
        imports: {
            autoImport: true,
        },
    },

    routeRules: {
        '/api/**': { cors: true, headers: corsHeaders },
        '/u/:id': { cors: true, headers: corsHeaders },
    },

    runtimeConfig: {
        public: {
            fileChunkSize: 25,
            returnHttps: 'auto',
            turnstile: {
                siteKey: '',
            },
            aiEnabled: true,
            site: {
                name: 'Fileship',
                description: 'An open-source, self-hosted file and media management platform.',
            },
        },
        turnstile: {
            secretKey: '',
        },
    },

    app: {
        buildAssetsDir: '/_fileship/',
        rootAttrs: {
            id: '__FileshipRoot',
        },
        head: {
            title: 'Fileship',
            htmlAttrs: {
                lang: 'en',
            },
            link: [
                {
                    rel: 'icon',
                    href: '/favicon.ico',
                },
                {
                    rel: 'apple-touch-icon',
                    href: '/apple-touch-icon.png',
                },
                {
                    rel: 'manifest',
                    href: '/manifest.json',
                },
            ],
        },
    },

    vite: {
        plugins: [tailwindcss()],
        build: {
            rollupOptions: {
                output: {
                    banner: `/*
* Fileship v${pkg.version}
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
                'dayjs/plugin/duration', // CJS
                'dayjs/plugin/relativeTime', // CJS
                'socket.io-client',
                'gsap',
                'gsap/SplitText',
                'filesize',
            ],
        },
    },

    compatibilityDate: '2024-09-04',
});
