import { join } from 'node:path';

import { app, BrowserWindow, Notification, shell } from 'electron';

import type { IpcEvents } from '../shared/ipc';

const DARK = '#0a0a0a';
const LIGHT = '#ffffff';

let mainWindow: BrowserWindow | null = null;
let isQuitting = false;
let hasShownTrayHint = false;

export const iconPath = () => join(app.getAppPath(), 'resources', 'icon.png');

export const rendererUrl = (route = '/') =>
    process.env.DESKTOP_RENDERER_URL
        ? `${process.env.DESKTOP_RENDERER_URL}${route}`
        : `app://fileship${route}`;

export const isAppUrl = (url: string) => {
    const origin = process.env.DESKTOP_RENDERER_URL ?? 'app://fileship';
    return url.startsWith(origin);
};

export const markQuitting = () => {
    isQuitting = true;
};

const hardenWebContents = (window: BrowserWindow) => {
    window.webContents.setWindowOpenHandler(({ url }) => {
        if (/^https?:\/\//.test(url)) shell.openExternal(url);
        return { action: 'deny' };
    });

    window.webContents.on('will-navigate', (event, url) => {
        if (!isAppUrl(url)) event.preventDefault();
    });
};

export const createMainWindow = (show: boolean) => {
    mainWindow = new BrowserWindow({
        width: 1100,
        height: 720,
        minWidth: 760,
        minHeight: 520,
        show: false,
        title: 'Fileship',
        icon: iconPath(),
        backgroundColor: DARK,
        autoHideMenuBar: true,
        webPreferences: {
            preload: join(__dirname, 'preload.cjs'),
            contextIsolation: true,
            sandbox: true,
            nodeIntegration: false,
        },
    });

    hardenWebContents(mainWindow);

    mainWindow.once('ready-to-show', () => {
        if (show) mainWindow?.show();
    });

    mainWindow.on('close', (event) => {
        if (isQuitting) return;

        event.preventDefault();
        mainWindow?.hide();

        if (!hasShownTrayHint && Notification.isSupported()) {
            hasShownTrayHint = true;
            new Notification({
                title: 'Fileship is still running',
                body: 'Use the tray icon or your hotkeys to capture. Right click it to quit.',
                icon: iconPath(),
            }).show();
        }
    });

    mainWindow.loadURL(rendererUrl());

    return mainWindow;
};

export const getMainWindow = () => mainWindow;

export const showMainWindow = (route?: string) => {
    if (!mainWindow) return;

    if (route) mainWindow.webContents.send('app:navigate', route);
    if (mainWindow.isMinimized()) mainWindow.restore();

    mainWindow.show();
    mainWindow.focus();
};

export const setWindowTheme = (isDark: boolean) => {
    for (const window of BrowserWindow.getAllWindows()) {
        if (!window.isDestroyed()) window.setBackgroundColor(isDark ? DARK : LIGHT);
    }
};

export const broadcast = <K extends keyof IpcEvents>(channel: K, payload: IpcEvents[K]) => {
    for (const window of BrowserWindow.getAllWindows()) {
        if (!window.isDestroyed()) window.webContents.send(channel, payload);
    }
};
