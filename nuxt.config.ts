import themes from './app/styles/themes.json';
import pkg from './package.json';

export default defineNuxtConfig({
    future: { compatibilityVersion: 4 },

    modules: [
        '@nuxtjs/turnstile',
        '@nuxt/eslint',
        '@nuxt/fonts',
        '@vueuse/nuxt',
        '@unocss/nuxt',
        '@nuxt/icon',
        '@vite-pwa/nuxt',
    ],

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
        },
        esbuild: {
            options: {
                target: 'esnext',
            },
        },
    },

    routeRules: {
        '/api/**': { cors: true },
        '/u/:id': { cors: true },
    },

    runtimeConfig: {
        public: {
            fileChunkSize: 10,
            returnHttps: '',
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
            meta: [
                {
                    name: 'theme-color',
                    content: '#5e58f9',
                },
            ],
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
            script: [
                {
                    innerHTML: `const themes=${JSON.stringify(themes)};Object.defineProperty(window,"theme",{get(){return localStorage.getItem("theme")},set(c){const e=themes[c]||themes.Fileship,s=[["--fs-background",e.background],["--fs-accent",e.accent],...e.overlays.map((t,o)=>[\`--fs-overlay-\${o+1}\`,t])];for(const[t,o]of s)document.documentElement.style.setProperty(t,o);localStorage.setItem("theme",c)}}),window.theme=localStorage.getItem("theme")||"Fileship";`,
                },
            ],
        },
    },

    vite: {
        $client: {
            resolve: {
                alias: {
                    '.prisma/client/index-browser':
                        './node_modules/.prisma/client/index-browser.js',
                },
            },
        },
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

    vue: {
        propsDestructure: true,
    },

    compatibilityDate: '2024-09-04',
});
