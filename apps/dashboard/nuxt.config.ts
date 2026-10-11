import tailwindcss from '@tailwindcss/vite';

import pkg from '../../package.json' with { type: 'json' };

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
        '@nuxtjs/color-mode',
        'vue-sonner/nuxt',
        'nuxt-lucide-icons',
    ].concat((process.env.ENABLE_PWA || 'true') === 'true' ? ['@vite-pwa/nuxt'] : []),

    components: [
        {
            path: '~/components',
            extensions: ['.vue'],
        },
    ],

    css: [
        '~/styles/tailwind.css',
        'katex/dist/katex.min.css',
        'highlight.js/styles/tokyo-night-dark.css',
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

            '0 * * * *': 'dir:clearTemp',
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
            registrationEnabled: false,
            site: {
                name: 'Fileship',
                description: 'An open-source, self-hosted file and media management platform.',
            },
        },
        turnstile: {
            secretKey: '',
        },
        registration: {
            permissions: 'UploadFiles,TakeNotes',
            usableSpace: -1,
        },
        dataDirectory: 'data',
        removeExifData: true,
        avatarMaxSize: 4,
        logLevel: 2,
        telemetryEnabled: true,
        updater: {
            url: 'http://updater-agent:3001',
            secret: 'dev-secret-token',
        },
        ai: {
            queueConcurrency: 1,
            inferenceThreads: 2,
            videoMaxFrames: 24,
            videoSampleIntervalSeconds: 10,
            jobLeaseMs: 15 * 60_000,
            jobDequeueRetries: 5,
        },
    },

    app: {
        buildAssetsDir: '/_hanzy_was_here/',
        rootAttrs: {
            id: '__fileship',
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
                '@lucide/vue',
                '@unovis/ts',
                '@unovis/vue',
                'class-variance-authority',
                'clsx',
                'dayjs', // CJS
                'dayjs/plugin/duration', // CJS
                'dayjs/plugin/relativeTime', // CJS
                'filesize',
                'highlight.js',
                'marked',
                'marked-katex-extension',
                'nanoid',
                'reka-ui',
                'socket.io-client',
                'tailwind-merge',
                'vue-input-otp',
            ],
        },
    },

    compatibilityDate: '2024-09-04',
});
