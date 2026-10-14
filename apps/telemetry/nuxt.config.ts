import pkg from '../../package.json' with { type: 'json' };

export default defineNuxtConfig({
    extends: ['@fileship/ui'],

    future: {
        compatibilityVersion: 5,
    },

    components: [
        {
            path: '~/components',
            extensions: ['.vue'],
        },
    ],

    css: ['~/styles/tailwind.css'],

    imports: {
        autoImport: true,
    },

    experimental: {
        nitroAutoImports: true,
    },

    nitro: {
        experimental: {
            tasks: true,
        },
        imports: {
            autoImport: true,
        },
        scheduledTasks: {
            '5 * * * *': 'stats:rollup',
            '30 3 * * *': 'db:prune',
        },
    },

    routeRules: {
        '/': { redirect: '/overview' },
    },

    runtimeConfig: {
        adminPassword: '',
        instanceRetentionDays: 90,
    },

    app: {
        buildAssetsDir: '/_hanzy_was_here/',
        rootAttrs: {
            id: '__fileship_telemetry',
        },
        head: {
            title: 'Fileship Telemetry',
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
            ],
        },
    },

    vite: {
        build: {
            rollupOptions: {
                output: {
                    banner: `/*
* Fileship Telemetry v${pkg.version}
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
                'dayjs',
                'dayjs/plugin/duration',
                'dayjs/plugin/localizedFormat',
                'dayjs/plugin/relativeTime',
                'filesize',
            ],
        },
    },

    compatibilityDate: '2024-09-04',
});
