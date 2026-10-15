import { app, Menu, nativeImage, Tray } from 'electron';

import { runAction } from './actions';
import { openDashboard } from './dashboard';
import { openDrive } from './drive';
import {
    abortRecording,
    getRecorderState,
    pauseRecording,
    resumeRecording,
    stopRecording,
} from './recording/recorder';
import { getSettings, getStore } from './store';
import { checkForUpdates, getUpdateStatus, installUpdate } from './updater';
import { iconPath, markQuitting, showMainWindow } from './windows';

let tray: Tray | null = null;

const buildMenu = () => {
    const isConnected = !!getStore().user;
    const update = getUpdateStatus();

    const recorder = getRecorderState();
    const isRecording = recorder.phase !== 'idle';

    const recordingItems: Electron.MenuItemConstructorOptions[] = isRecording
        ? [
              {
                  label: recorder.kind === 'gif' ? 'Stop GIF recording' : 'Stop recording',
                  enabled: recorder.phase !== 'processing',
                  click: () => stopRecording(),
              },
              recorder.phase === 'paused'
                  ? { label: 'Resume', click: () => resumeRecording() }
                  : {
                        label: 'Pause',
                        enabled: recorder.phase === 'recording',
                        click: () => pauseRecording(),
                    },
              {
                  label: 'Discard recording',
                  enabled: recorder.phase !== 'processing',
                  click: () => abortRecording(),
              },
              { type: 'separator' },
          ]
        : [];

    return Menu.buildFromTemplate([
        ...recordingItems,
        {
            label: 'Capture',
            submenu: [
                { label: 'Region', click: () => runAction('captureRegion') },
                { label: 'Entire screen', click: () => runAction('captureFullscreen') },
                { label: 'Active window', click: () => runAction('captureWindow') },
                { label: 'Active monitor', click: () => runAction('captureMonitor') },
                { label: 'Last region', click: () => runAction('captureLastRegion') },
            ],
        },
        {
            label: 'Record',
            enabled: !isRecording,
            submenu: [
                { label: 'Screen recording', click: () => runAction('recordVideo') },
                { label: 'GIF', click: () => runAction('recordGif') },
            ],
        },
        {
            label: 'Upload',
            enabled: isConnected,
            submenu: [
                { label: 'File...', click: () => runAction('uploadFile') },
                { label: 'From clipboard', click: () => runAction('uploadClipboard') },
                {
                    label: 'Shorten link from clipboard',
                    click: () => runAction('shortenClipboard'),
                },
            ],
        },
        { type: 'separator' },
        { label: 'Open Fileship', click: () => showMainWindow() },
        {
            label: 'Open Fileship drive',
            enabled: isConnected && getSettings().drive.enabled,
            click: () => openDrive(),
        },
        {
            label: 'Open dashboard',
            enabled: isConnected,
            click: () => openDashboard(),
        },
        { type: 'separator' },
        update.state === 'ready'
            ? { label: `Restart to update to ${update.version}`, click: installUpdate }
            : { label: 'Check for updates', click: () => checkForUpdates() },
        { label: 'Settings', click: () => showMainWindow('/settings/general') },
        { type: 'separator' },
        {
            label: 'Quit Fileship',
            click: () => {
                markQuitting();
                app.quit();
            },
        },
    ]);
};

export const refreshTray = () => {
    if (!tray) return;

    tray.setContextMenu(buildMenu());
    const tooltip = getStore().user ? `Fileship · ${getStore().siteName}` : 'Fileship';

    tray.setToolTip(
        getRecorderState().phase === 'idle' ? tooltip : 'Fileship · Recording, click to stop',
    );
};

export const createTray = () => {
    tray = new Tray(nativeImage.createFromPath(iconPath()).resize({ width: 16, height: 16 }));

    tray.on('click', () => {
        if (getRecorderState().phase === 'idle') return runAction('captureRegion');
        stopRecording();
    });
    tray.on('middle-click', () => runAction('uploadClipboard'));
    tray.on('double-click', () => showMainWindow());
    refreshTray();

    return tray;
};
