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
        'nuxt-icon',
    ],
    css: ['~/styles/main.css', '@unocss/reset/tailwind.css'],
    fonts: {
        families: [
            {
                name: 'Montserrat',
                provider: 'google',
            },
        ],
    },
    devtools: {
        enabled: true,
    },
    vue: {
        propsDestructure: true,
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
            script: [
                {
                    innerHTML: `const themes=${JSON.stringify(themes)};Object.defineProperty(window,"theme",{get(){return localStorage.getItem("theme")},set(e){const o=themes[e]||themes.Fileship,t=[["--color-fs-1",o[1]],["--color-fs-2",o[2]],["--color-fs-3",o[3]],["--color-fs-4",o[4]],["--color-fs-5",o[5]],["--color-fs-accent",o.accent]];for(const[c,l]of t)document.documentElement.style.setProperty(c,l);localStorage.setItem("theme",e)}}),window.theme=localStorage.getItem("theme")||"Fileship";`,
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
                    assetFileNames: '_fileship/module.[ext]',
                    chunkFileNames: '_fileship/module.js',
                    entryFileNames: '_fileship/module.js',
                    banner: `/*
* Fileship v${pkg.version}
* https://fileship.hanzy.dev
* (c) 2024-present Hànzy and Fileship contributors
* @license MIT
*/`,
                },
            },
        },
    },
    compatibilityDate: '2024-07-04',
});
