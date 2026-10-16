import { app } from 'electron';
import log from 'electron-log/main';
import electronUpdater from 'electron-updater';

import type { UpdateStatus } from '../shared/ipc';
import { t } from './i18n';
import { refreshTray } from './tray';
import { broadcast, markQuitting, notify } from './windows';

const { autoUpdater } = electronUpdater;

const CHECK_INTERVAL = 4 * 60 * 60 * 1000;
const RETRY_DELAY = 15 * 60 * 1000;

let status: UpdateStatus = { state: 'idle' };
let isBusy = () => false;
let installWhenIdle = false;
let retryTimer: NodeJS.Timeout | null = null;

const describeError = (error: any) => {
    const message = String(error?.message ?? '');

    if (/^404\b/.test(message)) return t('updater.noReleases');
    if (/ENOTFOUND|ECONNREFUSED|ETIMEDOUT|net::ERR_/.test(message)) {
        return t('updater.unreachable');
    }

    return message.split('\n')[0]!.slice(0, 200) || t('updater.checkFailed');
};

const setStatus = (next: UpdateStatus) => {
    const isChanged = next.state !== status.state;

    status = next;
    broadcast('updates:status', status);
    if (isChanged) refreshTray();
};

export const getUpdateStatus = () => status;

export const setBusyCheck = (check: () => boolean) => {
    isBusy = check;
};

const install = () => {
    markQuitting();
    autoUpdater.quitAndInstall(true, true);
};

export const installUpdate = () => {
    if (status.state !== 'ready') return;

    if (isBusy()) {
        installWhenIdle = true;
        return;
    }

    setImmediate(install);
};

export const notifyIdle = () => {
    if (installWhenIdle && !isBusy()) setImmediate(install);
};

export const checkForUpdates = async () => {
    if (!app.isPackaged) {
        setStatus({ state: 'unsupported' });
        return;
    }

    if (['checking', 'downloading', 'ready'].includes(status.state)) return;

    if (retryTimer) clearTimeout(retryTimer);
    retryTimer = null;

    try {
        await autoUpdater.checkForUpdates();
    } catch (error: any) {
        log.warn('Update check failed', error);
        setStatus({ state: 'error', error: describeError(error) });
        retryTimer = setTimeout(checkForUpdates, RETRY_DELAY);
    }
};

export const startUpdater = () => {
    autoUpdater.logger = log;
    autoUpdater.autoDownload = true;
    autoUpdater.autoInstallOnAppQuit = true;
    autoUpdater.allowDowngrade = false;

    autoUpdater.on('checking-for-update', () => setStatus({ state: 'checking' }));
    autoUpdater.on('update-not-available', () => setStatus({ state: 'latest' }));

    autoUpdater.on('update-available', (info) =>
        setStatus({ state: 'downloading', version: info.version, progress: 0 }),
    );

    autoUpdater.on('download-progress', (progress) =>
        setStatus({ ...status, state: 'downloading', progress: Math.round(progress.percent) }),
    );

    autoUpdater.on('update-downloaded', (info) => {
        setStatus({ state: 'ready', version: info.version });

        notify(
            t('updater.readyTitle', { version: info.version }),
            t('updater.readyBody'),
            installUpdate,
        );
    });

    autoUpdater.on('error', (error) => {
        log.warn('Updater error', error);
        setStatus({ state: 'error', version: status.version, error: describeError(error) });
    });

    if (!app.isPackaged) {
        setStatus({ state: 'unsupported' });
        return;
    }

    setTimeout(checkForUpdates, 10_000);
    setInterval(checkForUpdates, CHECK_INTERVAL);
};
