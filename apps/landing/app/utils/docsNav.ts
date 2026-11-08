export interface DocsNavGroup {
    key: string;
    folders: string[];
    items: string[];
}

export const DOCS_NAV: DocsNavGroup[] = [
    {
        key: 'gettingStarted',
        folders: ['install'],
        items: [
            '/docs/introduction',
            '/docs/install/requirements',
            '/docs/installation',
            '/docs/install/windows',
            '/docs/install/without-docker',
            '/docs/install/first-steps',
        ],
    },
    {
        key: 'proxy',
        folders: ['proxy'],
        items: [
            '/docs/proxy/overview',
            '/docs/proxy/caddy',
            '/docs/proxy/nginx',
            '/docs/proxy/apache',
            '/docs/proxy/traefik',
            '/docs/proxy/iis',
            '/docs/proxy/cloudflare-tunnel',
            '/docs/proxy/ssl',
            '/docs/proxy/custom-domains',
        ],
    },
    {
        key: 'configuration',
        folders: ['configuration'],
        items: [
            '/docs/configuration',
            '/docs/configuration/environment',
            '/docs/configuration/instance-settings',
            '/docs/configuration/storage',
            '/docs/configuration/email',
            '/docs/configuration/single-sign-on',
            '/docs/configuration/registration',
            '/docs/configuration/trash-and-retention',
            '/docs/configuration/logs',
        ],
    },
    {
        key: 'features',
        folders: ['features'],
        items: ['/docs/ai', '/docs/link-previews'],
    },
    {
        key: 'apps',
        folders: ['uploaders', 'api'],
        items: [
            '/docs/desktop',
            '/docs/browser-extension',
            '/docs/mobile',
            '/docs/uploaders/sharex',
            '/docs/uploaders/macos',
            '/docs/uploaders/linux',
            '/docs/uploaders/flameshot',
            '/docs/uploaders/ios-shortcuts',
            '/docs/uploaders/android',
            '/docs/uploaders/curl',
            '/docs/api/uploads',
        ],
    },
    {
        key: 'integrations',
        folders: [],
        items: ['/docs/webhooks', '/docs/telemetry'],
    },
    {
        key: 'updating',
        folders: ['updating', 'backups'],
        items: [
            '/docs/updating',
            '/docs/updating/without-docker',
            '/docs/upgrading',
            '/docs/migrating',
            '/docs/backups/database',
        ],
    },
    {
        key: 'troubleshooting',
        folders: ['troubleshooting'],
        items: ['/docs/troubleshooting/errors'],
    },
    {
        key: 'more',
        folders: [],
        items: ['/docs/browser-extension-privacy'],
    },
];
