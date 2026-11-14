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

import { type Channel, pickRelease, type Release } from './releases';

const rootDir = join(process.cwd(), '..', '..');
const EXPERIMENTAL_COMPOSE_FILE = join(process.cwd(), 'compose', 'experimental.yml');
const ML_COMPOSE_FILE = join(process.cwd(), 'compose', 'fileship-ml.yml');

const IS_DEV = process.env.DEV_MODE === 'true';
const AUTOUPDATER_ENABLED = process.env.AUTOUPDATER_ENABLED === 'true';
const AUTOUPDATER_CRON = process.env.AUTOUPDATER_CRON || '0 3 * * *';

const PORT = +process.env.PORT! || 3001;
const SECRET_TOKEN = process.env.SECRET_TOKEN || (IS_DEV ? 'dev-secret-token' : '');
const COMPOSE_FILE = process.env.COMPOSE_FILE || 'docker-compose.yml';
const FILESHIP_URL = process.env.FILESHIP_URL || 'http://proxy:3000';

const COLORS = ['blue', 'green'] as const;
const HEALTH_TIMEOUT = 3 * 60 * 1000;
const POSTGRES_HEALTH_TIMEOUT = 60 * 60 * 1000;
const RESTORE_WAIT_TIMEOUT = 10 * 60 * 1000;

type Color = (typeof COLORS)[number];
type UpdateStatus = 'Pulling' | 'Starting' | 'Switching' | 'Success' | 'Error';

let isUpdating = false;
let pinnedVersion: string | undefined;

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

const getClientInfo = () => fsFetch<{ version: string; channel: Channel }>('/healthz');

const isUpdateAvailable = async () => {
    const { version: currentVersion, channel } = await getClientInfo();

    const releases = await ghFetch<Release[]>('/repos/hanzydev/Fileship/releases', {
        query: { per_page: 30 },
    });

    const latestVersion = pickRelease(releases, channel);
    if (!latestVersion) return { hasUpdate: false, latestVersion: null, currentVersion, channel };

    const isImageAvailable = await isImageAvailableOnGHCR(latestVersion);

    return {
        hasUpdate: isImageAvailable && (IS_DEV || semver.gt(latestVersion, currentVersion)),
        latestVersion,
        currentVersion,
        channel,
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

    const args = ['compose', '-p', project!, '--project-directory', projectDir, '-f', COMPOSE_FILE];

    const { stdout: services } = await execa('docker', [...args, 'config', '--services'], {
        cwd: rootDir,
    });

    return [
        ...args,
        ...(services.split('\n').includes('fileship-ml') ? [] : ['-f', ML_COMPOSE_FILE]),
        ...(pinnedVersion ? ['-f', EXPERIMENTAL_COMPOSE_FILE] : []),
    ];
};

const compose = async (...args: string[]) =>
    execa('docker', [...(await getComposeArgs()), '--profile', 'green', ...args], {
        cwd: rootDir,
        env: { FILESHIP_VERSION: pinnedVersion },
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

const docker = async (...args: string[]) => (await execa('docker', args)).stdout.trim();

const serviceContainer = async (service: string) =>
    (await compose('ps', '-q', service)).stdout.trim();

const serviceImage = async (service: string) => {
    const { stdout } = await compose('config', '--format', 'json');
    return JSON.parse(stdout).services[service].image as string;
};

const pgVersionOf = async (kind: 'container' | 'image', target: string) => {
    const env = await docker(
        kind,
        'inspect',
        target,
        '--format',
        '{{range .Config.Env}}{{println .}}{{end}}',
    );
    return env.split('\n').find((line) => line.startsWith('PG_VERSION='));
};

const waitUntilPostgresHealthy = async () => {
    const startedAt = Date.now();

    while (Date.now() - startedAt < POSTGRES_HEALTH_TIMEOUT) {
        const container = await serviceContainer('postgres');
        const [status, health] = (
            await docker(
                'container',
                'inspect',
                container,
                '--format',
                '{{.State.Status}}|{{if .State.Health}}{{.State.Health.Status}}{{end}}',
            )
        ).split('|');

        if (health === 'healthy' || (status === 'running' && !health)) return true;
        if (['exited', 'dead', 'restarting'].includes(status!)) return false;

        await new Promise((resolve) => setTimeout(resolve, 2_000));
    }

    return false;
};

const updatePostgres = async () => {
    await compose('pull', '--ignore-buildable', 'postgres');

    const container = await serviceContainer('postgres');
    const target = await pgVersionOf('image', await serviceImage('postgres'));

    if (container && (await pgVersionOf('container', container)) === target) return;

    consola.info(`Restarting postgres on ${target?.slice('PG_VERSION='.length)}...`);

    await compose('up', '-d', '--no-deps', '--no-build', 'postgres');

    if (!(await waitUntilPostgresHealthy())) {
        throw new Error('postgres failed to become healthy');
    }
};

const updateSelf = async () => {
    if (IS_DEV) return;

    await compose('pull', 'updater-agent');

    const latest = await docker(
        'image',
        'inspect',
        await serviceImage('updater-agent'),
        '--format',
        '{{.Id}}',
    );
    const current = await docker('container', 'inspect', hostname(), '--format', '{{.Image}}');

    if (latest === current) return;

    consola.info('Restarting the updater agent on its new version...');

    await docker(
        'run',
        '-d',
        '--rm',
        '--volumes-from',
        hostname(),
        '-w',
        rootDir,
        '-e',
        'SECRET_TOKEN',
        ...(pinnedVersion ? ['-e', `FILESHIP_VERSION=${pinnedVersion}`] : []),
        latest,
        'docker',
        ...(await getComposeArgs()),
        'up',
        '-d',
        '--no-deps',
        '--no-build',
        '--force-recreate',
        'updater-agent',
    );
};

const ensureMLService = async () => {
    if (IS_DEV || isUpdating) return;

    isUpdating = true;

    try {
        const { version, channel } = await getClientInfo();
        pinnedVersion = channel === 'experimental' ? version : undefined;

        await compose('up', '-d', '--no-deps', '--no-build', '--no-recreate', 'fileship-ml');
    } finally {
        isUpdating = false;
        pinnedVersion = undefined;
    }
};

const startMLService = () =>
    ensureMLService().catch((error) => {
        consola.warn('Starting the AI service failed, trying again in a minute:', error);
        setTimeout(startMLService, 60_000);
    });

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
        const update = await isUpdateAvailable();

        if (!update.hasUpdate) {
            throw new Error(
                `No version newer than ${update.currentVersion} on the ${update.channel} channel`,
            );
        }

        pinnedVersion = update.channel === 'experimental' ? update.latestVersion! : undefined;

        const running = await getRunningColors();
        target = running.includes('green') ? 'blue' : 'green';

        const previous = running.filter((color) => color !== target);

        await onStatus?.('Pulling');

        consola.info('Checking for a new postgres image...');

        await updatePostgres();

        consola.info('Updating the AI service...');

        await compose('pull', '--ignore-buildable', 'fileship-ml');
        await compose('up', '-d', '--no-deps', '--no-build', 'fileship-ml');

        consola.info(`Pulling fileship ${update.latestVersion}...`);

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

        await updateSelf().catch((error) =>
            consola.error('Failed to update the updater agent:', error),
        );

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
        pinnedVersion = undefined;
    }
};

cron.schedule(AUTOUPDATER_CRON, async () => {
    if (!AUTOUPDATER_ENABLED) {
        return consola.info(
            'Auto updater is disabled. To enable, set the AUTOUPDATER_ENABLED environment variable to "true"',
        );
    }

    const updateAvailable = await isUpdateAvailable().catch((error) => {
        consola.warn('Could not check for updates, skipping this auto-update:', error);
        return null;
    });

    if (!updateAvailable) return;

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

    startMLService();
});
