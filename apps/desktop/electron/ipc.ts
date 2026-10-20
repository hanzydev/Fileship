import { existsSync, readdirSync } from 'node:fs';
import { resolve } from 'node:path';

import { app, dialog, globalShortcut, ipcMain, nativeTheme, shell } from 'electron';
import log from 'electron-log/main';

import { type Folder, INVOKE_CHANNELS, type IpcInvoke } from '../shared/ipc';
import { checkInstance, getAccount, login, logout } from './account';
import { runAction, uploadPaths } from './actions';
import { api } from './api';
import { writeText } from './clipboard';
import { closeDashboard, openDashboard } from './dashboard';
import { drivePathFor, driveRoot, getDriveStatus, openDrive, startDrive, stopDrive } from './drive';
import { clearHistory, getHistory, listHistory, removeHistory } from './history';
import {
    getHotkeyStatus,
    isSnippingToolKeyEnabled,
    registerHotkeys,
    setSnippingToolKey,
} from './hotkeys';
import { getLanguage, t, updateLanguage } from './i18n';
import { finishRegionCapture, markOverlayLoaded, markOverlayReady } from './overlay';
import { downloadFfmpeg, getFfmpegStatus, resetFfmpegProbe } from './recording/ffmpeg';
import {
    controlRecording,
    getRecorderState,
    markRecorderReady,
    recordingPreview,
    setRecordingMute,
    showRecorderWindow,
    writeRecordingAudio,
} from './recording/recorder';
import { registerShellMenu } from './shell';
import { getSettings, updateSettings } from './store';
import { refreshTray } from './tray';
import { checkForUpdates, getUpdateStatus, installUpdate } from './updater';
import { getUploadProgress, resetUploadSettings } from './upload';
import { broadcast, isAppUrl, notify, openWebUrl, setWindowTheme } from './windows';

type Handlers = {
    [K in keyof IpcInvoke]: (
        this: Electron.IpcMainInvokeEvent,
        ...args: Parameters<IpcInvoke[K]>
    ) => ReturnType<IpcInvoke[K]> | Promise<ReturnType<IpcInvoke[K]>>;
};

const pausingHotkeys = new Set<Electron.WebContents>();

const watchHotkeyPause = (sender: Electron.WebContents) => {
    if (pausingHotkeys.has(sender)) return;

    pausingHotkeys.add(sender);
    sender.once('destroyed', () => {
        pausingHotkeys.delete(sender);
        registerHotkeys();
    });
};

const runInBackground = (label: string, task: () => Promise<unknown>) => {
    task().catch((error) => {
        log.error(`${label} failed`, error);
        notify(t('app.somethingWentWrong'), error.message);
    });
};

const applySettings = () => {
    const { startAtLogin, theme } = getSettings();

    if (app.isPackaged) app.setLoginItemSettings({ openAtLogin: startAtLogin, args: ['--hidden'] });
    nativeTheme.themeSource = theme;

    if (updateLanguage()) {
        refreshTray();
        registerShellMenu();
        broadcast('app:language', getLanguage());
    }
};

const handlers: Handlers = {
    'instance:check': (url) => checkInstance(url),
    'account:get': async () => {
        const account = await getAccount();
        refreshTray();
        return account;
    },
    'account:login': async (url) => {
        const account = await login(url);
        resetUploadSettings();
        refreshTray();
        startDrive();
        return account;
    },
    'account:logout': async () => {
        await closeDashboard();
        await stopDrive({ remove: true, forget: true });
        await logout();
        refreshTray();
    },
    'settings:get': () => getSettings(),
    'settings:set': async (patch) => {
        const previous = getSettings();
        const previousPath = previous.recording.ffmpegPath;
        const settings = updateSettings(patch);

        if (settings.drive.enabled !== previous.drive.enabled) {
            if (settings.drive.enabled) startDrive();
            else await stopDrive({ remove: true });
        }
        if (settings.recording.ffmpegPath !== previousPath) resetFfmpegProbe();
        applySettings();
        if ('hotkeys' in patch || 'hotkeysEnabled' in patch) registerHotkeys();
        return settings;
    },
    'app:info': () => ({ version: app.getVersion(), isPackaged: app.isPackaged }),
    'app:language': () => getLanguage(),
    'app:openDashboard': (route) => openDashboard(route),
    'window:theme': (isDark) => setWindowTheme(isDark),
    'updates:get': () => getUpdateStatus(),
    'updates:check': () => {
        checkForUpdates();
    },
    'updates:install': () => installUpdate(),
    'capture:run': (action) => {
        runAction(action);
    },
    'overlay:loaded': () => markOverlayLoaded(),
    'overlay:ready': () => markOverlayReady(),
    'overlay:finish': (result) => {
        runInBackground('Capture', () => finishRegionCapture(result));
    },
    'history:list': (offset, limit) => listHistory(offset, limit),
    'history:copy': async (id) => {
        const item = getHistory(id);
        const url = item?.shortUrl ?? item?.url;
        if (url) await writeText(url);
    },
    'history:open': (id) => {
        const item = getHistory(id);
        const url = item?.shortUrl ?? item?.url;
        if (url) openWebUrl(url);
    },
    'history:showFile': (id) => {
        const item = getHistory(id);
        if (item?.localPath) shell.showItemInFolder(item.localPath);
    },
    'history:showInDrive': (id) => {
        const fileId = getHistory(id)?.fileId;
        const path = fileId && drivePathFor(fileId);

        if (path) shell.showItemInFolder(path);
        else openDrive();
    },
    'history:remove': (id) => removeHistory(id),
    'history:clear': () => clearHistory(),
    'uploads:paths': (paths) => {
        uploadPaths(paths);
    },
    'uploads:progress': () => getUploadProgress(),
    'hotkeys:status': () => getHotkeyStatus(),
    'hotkeys:pause': function (paused) {
        if (!paused) return registerHotkeys();

        watchHotkeyPause(this.sender);
        globalShortcut.unregisterAll();
        return getHotkeyStatus();
    },
    'hotkeys:snippingTool': () => isSnippingToolKeyEnabled(),
    'hotkeys:setSnippingTool': (enabled) => setSnippingToolKey(enabled),
    'folders:list': async () => {
        const { items } = await api<{ items: Folder[] }>('/api/folders?limit=200&sort=name:asc');
        return items.map(({ id, name }) => ({ id, name }));
    },
    'dialog:pickFolder': async () => {
        const result = await dialog.showOpenDialog({ properties: ['openDirectory'] });
        return result.filePaths[0] ?? null;
    },
    'dialog:pickFfmpeg': async () => {
        const result = await dialog.showOpenDialog({
            title: t('app.chooseFfmpeg'),
            properties: ['openFile'],
            filters: [{ name: 'ffmpeg', extensions: ['exe'] }],
        });

        return result.filePaths[0] ?? null;
    },
    'ffmpeg:status': () => getFfmpegStatus(),
    'ffmpeg:download': () => {
        downloadFfmpeg();
    },
    'recording:preview': () => recordingPreview(),
    'recorder:get': () => getRecorderState(),
    'recorder:ready': (error) => markRecorderReady(error),
    'recorder:audio': (track, chunk) => writeRecordingAudio(track, chunk),
    'recorder:control': (action) => {
        runInBackground('Recorder', () => controlRecording(action));
    },
    'recorder:mute': (source, muted) => setRecordingMute(source, muted),
    'recorder:visible': function () {
        showRecorderWindow(this.sender);
    },
    'drive:status': () => getDriveStatus(),
    'drive:open': () => {
        openDrive();
    },
    'drive:setFolder': async (folder) => {
        if (folder && resolve(folder) === resolve(driveRoot())) return getDriveStatus();

        if (folder && existsSync(folder) && readdirSync(folder).length > 0) {
            throw new Error(t('app.driveFolderNotEmpty'));
        }

        await stopDrive({ remove: true });
        updateSettings({ drive: { ...getSettings().drive, folder } });
        await startDrive();
        return getDriveStatus();
    },
};

export const registerIpc = () => {
    applySettings();

    for (const channel of INVOKE_CHANNELS) {
        ipcMain.handle(channel, (event, ...args) => {
            if (!isAppUrl(event.senderFrame?.url ?? '')) throw new Error('Not allowed');
            return (handlers[channel] as (...values: unknown[]) => unknown).call(event, ...args);
        });
    }
};
