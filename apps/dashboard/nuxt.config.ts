import { copyFile } from 'node:fs/promises';
import { createRequire } from 'node:module';
import { join } from 'node:path';

import ffprobe from '@ffprobe-installer/ffprobe';

import pkg from '../../package.json' with { type: 'json' };

const aiWorkerPath = createRequire(import.meta.url).resolve('@fileship/ai/worker');

const corsHeaders = {
    'Access-Control-Allow-Origin': '*',
    'Access-Control-Allow-Methods': '*',
    'Access-Control-Allow-Headers': '*',
    'Access-Control-Expose-Headers': '*',
};

export default defineNuxtConfig({
    extends: ['@fileship/ui'],

    future: {
        compatibilityVersion: 5,
    },

    modules: ['@nuxtjs/turnstile'].concat(
        (process.env.ENABLE_PWA || 'true') === 'true' ? ['@vite-pwa/nuxt'] : [],
    ),

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
            '30 * * * *': 'db:purgeTrash',
            '* * * * *': 'search:refresh',
            '15 4 * * *': 'ai:cleanup',
            '0 */6 * * *': 'telemetry:collect',
        },
        framework: {
            name: pkg.name,
            version: pkg.version,
        },
        imports: {
            autoImport: true,
        },
        externals: {
            traceInclude: [ffprobe.path, aiWorkerPath],
        },
        hooks: {
            compiled: async (nitro) => {
                if (!nitro.options.dev)
                    await copyFile(
                        aiWorkerPath,
                        join(nitro.options.output.serverDir, 'worker.mjs'),
                    );
            },
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
            },
        },
        turnstile: {
            secretKey: '',
        },
        registration: {
            permissions: 'UploadFiles,TakeNotes,ShortenLinks',
            usableSpace: -1,
        },
        dataDirectory: 'data',
        removeExifData: true,
        avatarMaxSize: 4,
        trashRetentionDays: 30,
        logLevel: 2,
        telemetryEnabled: true,
        telemetryUrl: '',
        updater: {
            url: 'http://updater-agent:3001',
            secret: 'dev-secret-token',
        },
        ai: {
            preset: 'medium',
            queueConcurrency: 1,
            inferenceThreads: 2,
            jobLeaseMs: 2 * 60_000,
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
                '@unovis/vue',
                'dayjs', // CJS
                'dayjs/plugin/duration', // CJS
                'dayjs/plugin/relativeTime', // CJS
                'filesize',
                'highlight.js',
                'marked',
                'marked-katex-extension',
                'nanoid',
                'socket.io-client',
            ],
        },
    },

    compatibilityDate: '2024-09-04',
});
