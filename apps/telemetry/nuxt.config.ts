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
        optimizeDeps: {
            include: [
                '@unovis/vue',
                'dayjs', // CJS
                'dayjs/plugin/duration', // CJS
                'dayjs/plugin/localizedFormat', // CJS
                'dayjs/plugin/relativeTime', // CJS
                'filesize',
            ],
        },
    },

    compatibilityDate: '2024-09-04',
});
