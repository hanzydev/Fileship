import fsp from 'node:fs/promises';
import { createRequire } from 'node:module';

import { join } from 'pathe';

import pkg from '../../package.json';

const require = createRequire(import.meta.url);

const corsHeaders = {
    'Access-Control-Allow-Origin': '*',
    'Access-Control-Allow-Methods': '*',
    'Access-Control-Allow-Credentials': 'true',
    'Access-Control-Allow-Headers': '*',
    'Access-Control-Expose-Headers': '*',
};

export default defineNuxtConfig({
    modules: [
        '@nuxtjs/turnstile',
        '@nuxt/eslint',
        '@nuxt/fonts',
        '@vueuse/nuxt',
        '@unocss/nuxt',
        '@nuxt/icon',
        'vue-sonner/nuxt',
    ].concat((process.env.ENABLE_PWA || 'true') === 'true' ? ['@vite-pwa/nuxt'] : []),

    css: [
        '~/styles/main.css',
        '@unocss/reset/tailwind.css',
        '~/styles/apexcharts.css',
        '~/styles/sonner.css',
        'highlight.js/styles/tokyo-night-dark.css',
    ],

    fonts: {
        families: [
            {
                name: 'Quicksand',
                provider: 'google',
                weights: [300, 400, 500, 600, 700],
            },
        ],
    },

    devtools: {
        enabled: true,
    },

    experimental: {
        clientNodeCompat: true,
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
            [process.env.DELETE_EXPIRED_CRON || '* * * * *']: 'db:deleteExpired',
            [process.env.GENERATE_STATS_CRON || '*/30 * * * *']: 'db:generateStats',
        },
        framework: {
            name: pkg.name,
            version: pkg.version,
        },
        hooks: {
            compiled: async () => {
                if (process.env.NODE_ENV !== 'production') return;

                const prismaEngineDirectory = join(
                    '..',
                    '..',
                    'node_modules',
                    '@prisma',
                    'engines',
                );
                const prismaEngineFiles = (await fsp.readdir(prismaEngineDirectory)).filter((f) =>
                    f.includes('engine'),
                );

                const compiledPrismaEngineDirectory = join(
                    '.output',
                    'server',
                    'node_modules',
                    '@prisma',
                    'engines',
                );

                await Promise.all(
                    prismaEngineFiles.map((file) =>
                        fsp.copyFile(
                            join(prismaEngineDirectory, file),
                            join(compiledPrismaEngineDirectory, file),
                        ),
                    ),
                );
            },
            'dev:reload': () => require('onnxruntime-node'),
        },
    },

    routeRules: {
        '/api/**': { cors: true, headers: corsHeaders },
        '/u/:id': { cors: true, headers: corsHeaders },
    },

    runtimeConfig: {
        public: {
            fileChunkSize: 25,
            returnHttps: '',
            turnstile: {
                siteKey: '',
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
    },

    compatibilityDate: '2024-09-04',
});
