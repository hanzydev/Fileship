import { copyFile } from 'node:fs/promises';
import { createRequire } from 'node:module';
import { dirname, join } from 'node:path';

import ffprobe from '@ffprobe-installer/ffprobe';

import pkg from '../../package.json' with { type: 'json' };

const aiWorkerPath = join(
    dirname(createRequire(import.meta.url).resolve('@fileship/ai/package.json')),
    'dist/worker.mjs',
);

const corsHeaders = {
    'Access-Control-Allow-Origin': '*',
    'Access-Control-Allow-Methods': '*',
    'Access-Control-Allow-Headers': '*',
    'Access-Control-Expose-Headers': '*',
};

const pwaEnabled = (process.env.ENABLE_PWA || 'true') === 'true';

export default defineNuxtConfig({
    extends: ['@fileship/ui'],

    future: {
        compatibilityVersion: 5,
    },

    modules: ['@nuxtjs/turnstile'].concat(pwaEnabled ? ['@vite-pwa/nuxt'] : []),

    pwa: {
        manifestFilename: 'manifest.json',
        manifest: {
            name: 'Fileship',
            short_name: 'Fileship',
            description: 'An open-source, self-hosted file and media management platform.',
            start_url: '/',
            scope: '/',
            display: 'standalone',
            background_color: '#0a0a0a',
            theme_color: '#0a0a0a',
            orientation: 'portrait-primary',
            icons: [
                { src: '/fileship-192x192.png', sizes: '192x192', type: 'image/png' },
                { src: '/fileship-512x512.png', sizes: '512x512', type: 'image/png' },
            ],
            screenshots: [
                {
                    src: '/previews/fileship-desktop.png',
                    sizes: '1920x911',
                    type: 'image/png',
                    form_factor: 'wide',
                    label: 'Fileship',
                },
                {
                    src: '/previews/fileship-mobile.png',
                    sizes: '437x911',
                    type: 'image/png',
                    form_factor: 'narrow',
                    label: 'Fileship',
                },
            ],
            share_target: {
                action: '/dashboard/share',
                method: 'POST',
                enctype: 'multipart/form-data',
                params: {
                    title: 'title',
                    text: 'text',
                    url: 'url',
                    files: [{ name: 'files', accept: ['*/*'] }],
                },
            },
        },
        workbox: {
            navigateFallback: null,
            importScripts: [`/share-target.js?v=${pkg.version}`],
        },
    },

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

    i18n: {
        locales: [
            { code: 'en', language: 'en-US', name: 'English', file: 'en.json' },
            { code: 'tr', language: 'tr-TR', name: 'Türkçe', file: 'tr.json' },
            { code: 'de', language: 'de-DE', name: 'Deutsch', file: 'de.json' },
            { code: 'fr', language: 'fr-FR', name: 'Français', file: 'fr.json' },
            { code: 'it', language: 'it-IT', name: 'Italiano', file: 'it.json' },
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
            asyncContext: true,
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
            link: [
                {
                    rel: 'icon',
                    href: '/favicon.ico',
                },
                {
                    rel: 'apple-touch-icon',
                    href: '/apple-touch-icon.png',
                },
                ...(pwaEnabled
                    ? [
                          {
                              rel: 'manifest' as const,
                              href: '/manifest.json',
                          },
                      ]
                    : []),
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
                'dayjs/locale/de', // CJS
                'dayjs/locale/fr', // CJS
                'dayjs/locale/it', // CJS
                'dayjs/locale/az', // CJS
                'dayjs/locale/ja', // CJS
                'dayjs/locale/ko', // CJS
                'dayjs/locale/tr', // CJS
                'dayjs/locale/zh', // CJS
                'dayjs/plugin/duration', // CJS
                'dayjs/plugin/localizedFormat', // CJS
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
