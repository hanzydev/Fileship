import { readFile } from 'node:fs/promises';
import { createServer } from 'node:http';

import { consola } from 'consola';
import { execa } from 'execa';
import {
    createApp,
    createError,
    createEventStream,
    createRouter,
    defineEventHandler,
    getRequestHeader,
    toNodeListener,
} from 'h3';
import cron from 'node-cron';
import { ofetch } from 'ofetch';
import { join } from 'pathe';
import semver from 'semver';

const rootDir = join(process.cwd(), '..', '..');

const IS_DEV = process.env.DEV_MODE === 'true';
const AUTOUPDATER_ENABLED = process.env.AUTOUPDATER_ENABLED === 'true';
const AUTOUPDATER_CRON = process.env.AUTOUPDATER_CRON || '0 3 * * *';

const PORT = +process.env.PORT! || 3001;
const SECRET_TOKEN = process.env.SECRET_TOKEN || 'dev-secret-token';
const COMPOSE_FILE = process.env.COMPOSE_FILE || 'docker-compose.yml';
const FILESHIP_URL = process.env.FILESHIP_URL || 'http://fileship:3000';

let isUpdating = false;

const ghFetch = ofetch.create({
    baseURL: 'https://api.github.com',
    headers: {
        'User-Agent': 'Fileship-Updater-Agent',
    },
});

const fsFetch = ofetch.create({
    baseURL: `${FILESHIP_URL}/api`,
});

const ghcrFetch = ofetch.create({
    baseURL: 'https://ghcr.io',
});

const isImageAvailableOnGHCR = async (version: string) => {
    try {
        const cleanVersion = semver.clean(version) || version;

        const { token } = await ghcrFetch<{ token: string }>(
            '/token?scope=repository:hanzydev/fileship:pull',
        );

        const manifest = await ghcrFetch(`/v2/hanzydev/fileship/manifests/${cleanVersion}`, {
            headers: {
                Authorization: `Bearer ${token}`,
                Accept: [
                    'application/vnd.oci.image.index.v1+json',
                    'application/vnd.oci.image.manifest.v1+json',
                    'application/vnd.docker.distribution.manifest.v2+json',
                    'application/vnd.docker.distribution.manifest.list.v2+json',
                ].join(', '),
            },
            ignoreResponseError: true,
        });

        return manifest && !manifest.errors;
    } catch {
        consola.warn(`Image tag ${version} is not yet available on GHCR.`);
        return false;
    }
};

const getClientVersion = async (): Promise<string> => {
    try {
        const { version } = await fsFetch<{ version: string }>('/healthz');
        return version;
    } catch {
        const rootPkgPath = join(rootDir, 'package.json');
        return JSON.parse(await readFile(rootPkgPath, 'utf-8')).version;
    }
};

const isUpdateAvailable = async () => {
    const currentVersion = await getClientVersion();

    const latestRelease = await ghFetch('/repos/hanzydev/Fileship/releases/latest', {
        ignoreResponseError: true,
    });

    const latestVersion = latestRelease?.tag_name?.replace(/^v/, '');
    if (!latestVersion) return { hasUpdate: false, latestVersion: null, currentVersion };

    const isImageAvailable = await isImageAvailableOnGHCR(latestVersion);

    return {
        hasUpdate:
            isImageAvailable &&
            (IS_DEV || !!(semver.valid(latestVersion) && semver.gt(latestVersion, currentVersion))),
        latestVersion,
        currentVersion,
    };
};

const handleUpdate = async (
    onStatus?: (status: 'Pulling' | 'Recreating' | 'Success' | 'Error') => Promise<void>,
) => {
    if (isUpdating) {
        consola.warn('An update is already in progress. Skipping this request.');
        return false;
    }

    isUpdating = true;

    consola.info('Starting update process...');

    try {
        consola.info('Pulling latest fileship image...');

        await onStatus?.('Pulling');
        await execa('docker', ['compose', '-f', COMPOSE_FILE, 'pull', 'fileship'], {
            cwd: rootDir,
        });

        consola.info('Recreating fileship container...');

        await onStatus?.('Recreating');
        await execa(
            'docker',
            ['compose', '-f', COMPOSE_FILE, 'up', '-d', '--no-deps', 'fileship'],
            { cwd: rootDir },
        );

        consola.success('Fileship updated successfully!');
        await onStatus?.('Success');

        return true;
    } catch (error) {
        consola.error('Update execution failed:', error);
        await onStatus?.('Error');

        return false;
    } finally {
        isUpdating = false;
    }
};

cron.schedule(AUTOUPDATER_CRON, async () => {
    if (!AUTOUPDATER_ENABLED) {
        return consola.info(
            'Auto updater is disabled. To enable, set the AUTOUPDATER_ENABLED environment variable to "true"',
        );
    }

    const updateAvailable = await isUpdateAvailable();
    if (!updateAvailable.hasUpdate) {
        return consola.info(
            `Fileship is already at the latest version (${updateAvailable.currentVersion})`,
        );
    }

    consola.info(`New version found: ${updateAvailable.latestVersion}. Executing auto-update...`);

    await handleUpdate();
});

const app = createApp();
const router = createRouter();

app.use(router);

router.use(
    '/api/**',
    defineEventHandler((event) => {
        const authHeader = getRequestHeader(event, 'Authorization');
        if (authHeader !== SECRET_TOKEN) {
            throw createError({ statusCode: 401, statusMessage: 'Unauthorized' });
        }
    }),
);

router.post(
    '/api/update',
    defineEventHandler((event) => {
        if (isUpdating) {
            throw createError({
                statusCode: 409,
                statusMessage: 'An update is already in progress',
            });
        }

        const eventStream = createEventStream(event);

        handleUpdate(async (status) => {
            await eventStream.push(JSON.stringify({ status }));
            if (['Success', 'Error'].includes(status)) await eventStream.close();
        });

        return eventStream.send();
    }),
);

router.get('/api/status', defineEventHandler(isUpdateAvailable));

const server = createServer(toNodeListener(app));

server.listen(PORT, '0.0.0.0', () => {
    consola.info(`Updater agent running on port ${PORT}`);
});
