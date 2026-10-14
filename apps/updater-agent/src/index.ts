import { readFile } from 'node:fs/promises';
import { createServer } from 'node:http';
import { hostname } from 'node:os';

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
const SECRET_TOKEN = process.env.SECRET_TOKEN || (IS_DEV ? 'dev-secret-token' : '');
const COMPOSE_FILE = process.env.COMPOSE_FILE || 'docker-compose.yml';
const FILESHIP_URL = process.env.FILESHIP_URL || 'http://proxy:3000';

const COLORS = ['blue', 'green'] as const;
const HEALTH_TIMEOUT = 3 * 60 * 1000;
const RESTORE_WAIT_TIMEOUT = 10 * 60 * 1000;

type Color = (typeof COLORS)[number];
type UpdateStatus = 'Pulling' | 'Starting' | 'Switching' | 'Success' | 'Error';

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

const getComposeArgs = async () => {
    const { stdout } = await execa('docker', [
        'inspect',
        hostname(),
        '--format',
        '{{ index .Config.Labels "com.docker.compose.project" }}|{{ index .Config.Labels "com.docker.compose.project.working_dir" }}',
    ]);

    const [project, workingDir] = stdout.trim().split('|');

    const projectDir = workingDir!
        .replace(/^([a-z]):[\\/]/i, (_, drive) => `/run/desktop/mnt/host/${drive.toLowerCase()}/`)
        .replaceAll('\\', '/');

    return ['compose', '-p', project!, '--project-directory', projectDir, '-f', COMPOSE_FILE];
};

const compose = async (...args: string[]) =>
    execa('docker', [...(await getComposeArgs()), '--profile', 'green', ...args], {
        cwd: rootDir,
    });

const getRunningColors = async () => {
    const { stdout } = await compose('ps', '--status', 'running', '--services');
    return COLORS.filter((color) => stdout.split('\n').includes(`fileship-${color}`));
};

const waitUntilHealthy = async (color: Color) => {
    const startedAt = Date.now();

    while (Date.now() - startedAt < HEALTH_TIMEOUT) {
        const health = await ofetch<{ ready: boolean }>(
            `http://fileship-${color}:3000/api/healthz`,
            { timeout: 5_000 },
        ).catch(() => null);

        if (health?.ready) return true;

        const { stdout: state } = await compose(
            'ps',
            '-a',
            '--format',
            '{{.State}}',
            `fileship-${color}`,
        );
        if (['exited', 'dead', 'restarting'].includes(state.trim())) return false;

        await new Promise((resolve) => setTimeout(resolve, 2_000));
    }

    return false;
};

const isFileshipBusy = async () => {
    const health = await fsFetch<{ busy?: boolean }>('/healthz').catch(() => null);
    return !!health?.busy;
};

const handleUpdate = async (onStatus?: (status: UpdateStatus) => Promise<void>) => {
    if (isUpdating) {
        consola.warn('An update is already in progress. Skipping this request.');
        return false;
    }

    isUpdating = true;

    consola.info('Starting update process...');

    let target: Color | null = null;
    let isSwitched = false;

    try {
        const running = await getRunningColors();
        target = running.includes('green') ? 'blue' : 'green';

        const previous = running.filter((color) => color !== target);

        consola.info(`Pulling latest fileship image...`);

        await onStatus?.('Pulling');
        await compose('pull', '--ignore-buildable', `fileship-${target}`);

        consola.info(`Starting fileship-${target} next to ${previous.join(', ') || 'nothing'}...`);

        await onStatus?.('Starting');
        await compose(
            'up',
            '-d',
            '--no-deps',
            '--no-build',
            '--force-recreate',
            `fileship-${target}`,
        );

        if (!(await waitUntilHealthy(target))) {
            throw new Error(`fileship-${target} failed to become healthy`);
        }

        consola.info(`fileship-${target} is healthy, switching traffic...`);

        await onStatus?.('Switching');

        isSwitched = true;

        await onStatus?.('Success');

        const waitStartedAt = Date.now();

        while ((await isFileshipBusy()) && Date.now() - waitStartedAt < RESTORE_WAIT_TIMEOUT) {
            consola.info('A backup is being restored, waiting before stopping the old version...');
            await new Promise((resolve) => setTimeout(resolve, 5_000));
        }

        for (const color of previous) {
            await compose('stop', `fileship-${color}`);
        }

        consola.success(`Fileship updated successfully, fileship-${target} is now serving.`);

        return true;
    } catch (error) {
        consola.error('Update execution failed:', error);

        if (isSwitched) return true;

        if (target) {
            consola.info(`Rolling back, stopping fileship-${target}...`);
            await compose('stop', `fileship-${target}`).catch(() => null);
        }

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

    if (await isFileshipBusy()) {
        return consola.info('A backup is being restored, skipping this auto-update.');
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
        if (!SECRET_TOKEN || authHeader !== SECRET_TOKEN) {
            throw createError({ status: 401, statusText: 'Unauthorized' });
        }
    }),
);

router.post(
    '/api/update',
    defineEventHandler((event) => {
        if (isUpdating) {
            throw createError({
                status: 409,
                statusText: 'An update is already in progress',
            });
        }

        const eventStream = createEventStream(event);

        handleUpdate(async (status) => {
            await eventStream.push(JSON.stringify({ status })).catch(() => null);
            if (['Success', 'Error'].includes(status)) await eventStream.close().catch(() => null);
        });

        return eventStream.send();
    }),
);

router.get('/api/status', defineEventHandler(isUpdateAvailable));

const server = createServer(toNodeListener(app));

server.listen(PORT, '0.0.0.0', () => {
    consola.info(`Updater agent running on port ${PORT}`);

    if (!SECRET_TOKEN) {
        consola.warn(
            'SECRET_TOKEN is not set, so the dashboard cannot start updates. Add it to the .env file next to docker-compose.yml.',
        );
    }
});
