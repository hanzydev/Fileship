import { copyFile } from 'node:fs/promises';
import { createRequire } from 'node:module';
import { dirname, join } from 'node:path';

import ffprobe from '@ffprobe-installer/ffprobe';

import pkg from '../../package.json' with { type: 'json' };

const packageRequire = createRequire(import.meta.url);

const aiWorkerPath = join(
    dirname(packageRequire.resolve('@fileship/ai/package.json')),
    'dist/worker.mjs',
);

const pdfPackagePath = dirname(packageRequire.resolve('@fileship/pdf/package.json'));
const pdfWorkerPath = join(pdfPackagePath, 'dist/pdf-worker.mjs');
const pdfiumPath = createRequire(join(pdfPackagePath, 'package.json')).resolve(
    '@embedpdf/pdfium/pdfium.wasm',
);

const pdfjsPath = dirname(packageRequire.resolve('pdfjs-dist/package.json'));
const pdfjsVersion = packageRequire('pdfjs-dist/package.json').version;

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
            '*/15 * * * *': 'backups:run',
            '* * * * *': 'search:refresh',
            '15 4 * * *': 'ai:cleanup',
            '0 */6 * * *': 'telemetry:collect',
            '*/10 * * * *': 'email:storageWarnings',
        },
        framework: {
            name: pkg.name,
            version: pkg.version,
        },
        imports: {
            autoImport: true,
        },
        externals: {
            traceInclude: [ffprobe.path, aiWorkerPath, pdfWorkerPath, pdfiumPath],
        },
        publicAssets: ['cmaps', 'iccs', 'standard_fonts', 'wasm'].map((dir) => ({
            baseURL: `_pdfjs/${pdfjsVersion}/${dir}`,
            dir: join(pdfjsPath, dir),
            maxAge: 60 * 60 * 24 * 365,
        })),
        hooks: {
            compiled: async (nitro) => {
                if (nitro.options.dev) return;

                await copyFile(aiWorkerPath, join(nitro.options.output.serverDir, 'worker.mjs'));
                await copyFile(
                    pdfWorkerPath,
                    join(nitro.options.output.serverDir, 'pdf-worker.mjs'),
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
        scheduledBackupsEnabled: true,
        logLevel: 2,
        telemetryEnabled: true,
        telemetryUrl: '',
        siteUrl: '',
        smtp: {
            host: '',
            port: 587,
            security: 'starttls',
            user: '',
            password: '',
            from: '',
        },
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
