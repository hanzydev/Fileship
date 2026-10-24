import { fileURLToPath } from 'node:url';

import Components from 'unplugin-vue-components/vite';
import { defineConfig } from 'wxt';

import tailwindcss from '@tailwindcss/vite';

import { LANGUAGES, messages } from './i18n/messages';

const LOCALE_FOLDERS: Partial<Record<(typeof LANGUAGES)[number], string>> = { zh: 'zh_CN' };

const fromRoot = (path: string) => fileURLToPath(new URL(path, import.meta.url));

const UI_COMPONENTS = fromRoot('../../packages/ui/app/components');
const UI_APP_COMPONENTS = ['EmptyState', 'PasswordInput', 'TextBlock'];

export default defineConfig({
    modules: ['@wxt-dev/module-vue'],
    targetBrowsers: ['chrome', 'firefox'],
    manifestVersion: 3,
    alias: {
        '#layers/ui': fromRoot('../../packages/ui'),
    },
    imports: {
        presets: [{ from: '#layers/ui/app/utils', imports: ['cn'] }],
    },
    vite: () => ({
        plugins: [
            tailwindcss(),
            Components({
                dirs: [UI_COMPONENTS, fromRoot('components')],
                globsExclude: [`${UI_COMPONENTS}/!(${UI_APP_COMPONENTS.join('|')}).vue`],
                directoryAsNamespace: true,
                collapseSamePrefixes: true,
                dts: fromRoot('.wxt/components.d.ts'),
                resolvers: [
                    (name) =>
                        /^I[A-Z]/.test(name)
                            ? { name: `${name.slice(1)}Icon`, from: '@lucide/vue' }
                            : undefined,
                ],
            }),
        ],
        resolve: {
            dedupe: ['vue'],
        },
    }),
    manifest: ({ browser }) => ({
        name: 'Fileship',
        description: '__MSG_extDescription__',
        default_locale: 'en',
        homepage_url: 'https://github.com/hanzydev/Fileship',
        permissions: [
            'activeTab',
            'clipboardWrite',
            'contextMenus',
            'notifications',
            'scripting',
            'storage',
            ...(browser === 'firefox' ? [] : ['offscreen']),
        ],
        optional_host_permissions: ['http://*/*', 'https://*/*'],
        ...(browser !== 'firefox' && { minimum_chrome_version: '116' }),
        ...(browser === 'firefox' && {
            browser_specific_settings: {
                gecko: {
                    id: 'fileship@hanzy.dev',
                    strict_min_version: '142.0',
                    data_collection_permissions: {
                        required: ['websiteContent', 'browsingActivity'],
                    },
                },
            },
        }),
    }),
    hooks: {
        'build:publicAssets': (_, files) => {
            for (const language of LANGUAGES)
                files.push({
                    relativeDest: `_locales/${LOCALE_FOLDERS[language] ?? language}/messages.json`,
                    contents: JSON.stringify({
                        extDescription: { message: messages[language].manifest.description },
                    }),
                });
        },
    },
    zip: {
        name: 'fileship',
        sourcesRoot: fromRoot('../..'),
        includeSources: [
            'apps/extension/**',
            'docs/browser-extension.md',
            'packages/ui/**',
            'packages/eslint-config/**',
            'package.json',
            'pnpm-lock.yaml',
            'pnpm-workspace.yaml',
            'turbo.json',
        ],
        excludeSources: ['**/node_modules/**', '**/.output/**', '**/.wxt/**', '**/.nuxt/**'],
    },
});
