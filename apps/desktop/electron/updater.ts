import { app } from 'electron';
import log from 'electron-log/main';
import electronUpdater from 'electron-updater';

import type { UpdateStatus } from '../shared/ipc';
import { broadcast, markQuitting, notify } from './windows';

const { autoUpdater } = electronUpdater;

const CHECK_INTERVAL = 4 * 60 * 60 * 1000;
const RETRY_DELAY = 15 * 60 * 1000;

let status: UpdateStatus = { state: 'idle' };
let isBusy = () => false;
let installWhenIdle = false;

const describeError = (error: any) => {
    const message = String(error?.message ?? '');

    if (/^404\b/.test(message)) return 'No releases are published yet';
    if (/ENOTFOUND|ECONNREFUSED|ETIMEDOUT|net::ERR_/.test(message)) {
        return 'Could not reach the update server';
    }

    return message.split('\n')[0]!.slice(0, 200) || 'Update check failed';
};

const setStatus = (next: UpdateStatus) => {
    status = next;
    broadcast('updates:status', status);
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

    try {
        await autoUpdater.checkForUpdates();
    } catch (error: any) {
        log.warn('Update check failed', error);
        setStatus({ state: 'error', error: describeError(error) });
        setTimeout(checkForUpdates, RETRY_DELAY);
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
            `Fileship ${info.version} is ready`,
            'Click to restart and update now, or it installs when you quit.',
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
