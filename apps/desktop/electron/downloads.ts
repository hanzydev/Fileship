import { randomUUID } from 'node:crypto';
import { basename, dirname, extname, join } from 'node:path';

import { app, shell } from 'electron';

import type { DownloadEntry } from '../shared/dashboard';
import { t } from './i18n';
import { notify } from './windows';

const HISTORY_LIMIT = 50;
const UPDATE_INTERVAL_MS = 250;

const EXECUTABLE_TYPES = new Set([
    'appimage',
    'application',
    'appref-ms',
    'appx',
    'appxbundle',
    'bat',
    'chm',
    'cmd',
    'com',
    'cpl',
    'deb',
    'desktop',
    'exe',
    'gadget',
    'hta',
    'img',
    'inf',
    'iso',
    'jar',
    'js',
    'jse',
    'lnk',
    'msc',
    'msi',
    'msix',
    'msixbundle',
    'msp',
    'pif',
    'ps1',
    'psm1',
    'reg',
    'rpm',
    'run',
    'scf',
    'scr',
    'settingcontent-ms',
    'sh',
    'url',
    'vbe',
    'vbs',
    'vhd',
    'vhdx',
    'ws',
    'wsc',
    'wsf',
    'wsh',
]);

type Download = { item: Electron.DownloadItem; path: string; entry: DownloadEntry };

type Watcher = {
    onChange: (entries: DownloadEntry[]) => void;
    isWatched: () => boolean;
};

const downloads = new Map<string, Download>();
const trackedSessions = new WeakSet<Electron.Session>();

let watcher: Watcher | null = null;
let lastFolder: string | null = null;
let updateTimer: NodeJS.Timeout | null = null;

const isActive = (download: Download) => download.entry.state === 'progressing';

const isExecutable = (path: string) => EXECUTABLE_TYPES.has(extname(path).slice(1).toLowerCase());

export const listDownloads = () => [...downloads.values()].map(({ entry }) => entry).reverse();

const emit = () => {
    if (updateTimer) clearTimeout(updateTimer);
    updateTimer = null;
    watcher?.onChange(listDownloads());
};

const scheduleEmit = () => {
    updateTimer ??= setTimeout(emit, UPDATE_INTERVAL_MS);
};

const trim = () => {
    for (const [id, download] of downloads) {
        if (downloads.size <= HISTORY_LIMIT) return;
        if (!isActive(download)) downloads.delete(id);
    }
};

const refresh = (download: Download, state: DownloadEntry['state']) => {
    const { item, entry } = download;

    entry.state = state;
    entry.received = item.getReceivedBytes();
    entry.total = item.getTotalBytes();
    entry.bytesPerSecond = state === 'progressing' ? item.getCurrentBytesPerSecond() : 0;
};

const announce = (download: Download) => {
    const { entry, path } = download;
    if (watcher?.isWatched() || entry.state === 'cancelled') return;

    const title = entry.state === 'completed' ? t('app.downloadComplete') : t('app.downloadFailed');
    notify(title, entry.name, () => shell.showItemInFolder(path));
};

const handleDownload = (_: Electron.Event, item: Electron.DownloadItem) => {
    let download: Download | null = null;

    item.setSaveDialogOptions({
        defaultPath: join(lastFolder ?? app.getPath('downloads'), item.getFilename()),
    });

    const register = () => {
        if (download) return download;

        const path = item.getSavePath();
        if (!path) return null;

        lastFolder = dirname(path);
        download = {
            item,
            path,
            entry: {
                id: randomUUID(),
                name: basename(path),
                state: 'progressing',
                received: 0,
                total: 0,
                bytesPerSecond: 0,
                startedAt: Date.now(),
            },
        };

        downloads.set(download.entry.id, download);
        trim();
        return download;
    };

    item.on('updated', (_, state) => {
        const current = register();
        if (!current) return;

        refresh(current, state);
        scheduleEmit();
    });

    item.once('done', (_, state) => {
        const current = register();
        if (!current) return;

        refresh(current, state);
        emit();
        announce(current);
    });
};

export const trackDownloads = (target: Electron.Session) => {
    if (trackedSessions.has(target)) return;

    trackedSessions.add(target);
    target.on('will-download', handleDownload);
};

export const watchDownloads = (value: Watcher) => {
    watcher = value;
};

export const hasActiveDownloads = () => [...downloads.values()].some(isActive);

export const downloadProgress = () => {
    const active = [...downloads.values()].filter(isActive);
    if (!active.length) return -1;
    if (active.some(({ entry }) => !entry.total)) return 2;

    const received = active.reduce((sum, { entry }) => sum + entry.received, 0);
    const total = active.reduce((sum, { entry }) => sum + entry.total, 0);
    return received / total;
};

export const showDownload = (id: string) => {
    const download = downloads.get(id);
    if (download) shell.showItemInFolder(download.path);
};

export const openDownload = async (id: string) => {
    const download = downloads.get(id);
    if (!download || download.entry.state !== 'completed') return;

    if (isExecutable(download.path) || (await shell.openPath(download.path))) {
        shell.showItemInFolder(download.path);
    }
};

export const cancelDownload = (id: string) => {
    const download = downloads.get(id);
    if (download && isActive(download)) download.item.cancel();
};

export const removeDownload = (id: string) => {
    const download = downloads.get(id);
    if (!download) return;

    if (isActive(download)) download.item.cancel();
    downloads.delete(id);
    emit();
};

export const clearDownloads = () => {
    for (const [id, download] of downloads) {
        if (!isActive(download)) downloads.delete(id);
    }

    emit();
};

export const cancelDownloads = () => {
    for (const download of downloads.values()) {
        if (isActive(download)) download.item.cancel();
    }

    downloads.clear();
    emit();
};
