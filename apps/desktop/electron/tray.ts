import { app, Menu, nativeImage, Tray } from 'electron';

import { runAction } from './actions';
import { openDashboard } from './dashboard';
import { getStore } from './store';
import { checkForUpdates, getUpdateStatus, installUpdate } from './updater';
import { iconPath, markQuitting, showMainWindow } from './windows';

let tray: Tray | null = null;

const buildMenu = () => {
    const isConnected = !!getStore().user;
    const update = getUpdateStatus();

    return Menu.buildFromTemplate([
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
    tray.setToolTip(getStore().user ? `Fileship · ${getStore().siteName}` : 'Fileship');
};

export const createTray = () => {
    tray = new Tray(nativeImage.createFromPath(iconPath()).resize({ width: 16, height: 16 }));

    tray.on('click', () => runAction('captureRegion'));
    tray.on('middle-click', () => runAction('uploadClipboard'));
    tray.on('double-click', () => showMainWindow());
    refreshTray();

    return tray;
};
