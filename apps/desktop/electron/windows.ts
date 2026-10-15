import { join } from 'node:path';

import { app, BrowserWindow, nativeTheme, Notification, shell } from 'electron';

import type { IpcEvents } from '../shared/ipc';

const DARK = '#0a0a0a';
const LIGHT = '#ffffff';

let mainWindow: BrowserWindow | null = null;
let isQuitting = false;
let hasShownTrayHint = false;

export const iconPath = () => join(app.getAppPath(), 'resources', 'icon.png');

export const themeBackground = () => (nativeTheme.shouldUseDarkColors ? DARK : LIGHT);

export const rendererUrl = (route = '/') =>
    process.env.DESKTOP_RENDERER_URL
        ? `${process.env.DESKTOP_RENDERER_URL}${route}`
        : `app://fileship${route}`;

export const isAppUrl = (url: string) => {
    const origin = process.env.DESKTOP_RENDERER_URL ?? 'app://fileship';
    return url.startsWith(origin);
};

export const openWebUrl = (url: string) => {
    try {
        const { protocol } = new URL(url);
        if (protocol === 'http:' || protocol === 'https:') shell.openExternal(url);
    } catch {
        return;
    }
};

export const webPreferences = (
    preferences: Electron.WebPreferences = {},
): Electron.WebPreferences => ({
    ...preferences,
    contextIsolation: true,
    sandbox: true,
    nodeIntegration: false,
    devTools: !app.isPackaged,
});

const WEB_PERMISSIONS = new Set(['clipboard-sanitized-write', 'fullscreen']);

export const restrictPermissions = (target: Electron.Session) => {
    target.setPermissionRequestHandler((_, permission, callback) =>
        callback(WEB_PERMISSIONS.has(permission)),
    );
    target.setPermissionCheckHandler((_, permission) => WEB_PERMISSIONS.has(permission));
};

export const markQuitting = () => {
    isQuitting = true;
};

export const hardenWebContents = (window: BrowserWindow) => {
    window.webContents.setWindowOpenHandler(({ url }) => {
        openWebUrl(url);
        return { action: 'deny' };
    });

    window.webContents.on('will-navigate', (event, url) => {
        if (!isAppUrl(url)) event.preventDefault();
    });
};

const createMainWindow = (route = '/') => {
    const window = new BrowserWindow({
        width: 1100,
        height: 720,
        minWidth: 760,
        minHeight: 520,
        show: false,
        title: 'Fileship',
        icon: iconPath(),
        backgroundColor: themeBackground(),
        autoHideMenuBar: true,
        webPreferences: webPreferences({ preload: join(__dirname, 'preload.cjs') }),
    });

    hardenWebContents(window);

    window.once('ready-to-show', () => {
        window.show();
        window.focus();
    });

    window.on('close', () => {
        if (isQuitting || hasShownTrayHint || !Notification.isSupported()) return;

        hasShownTrayHint = true;
        notify(
            'Fileship is still running',
            'Use the tray icon or your hotkeys to capture. Right click it to quit.',
        );
    });

    window.on('closed', () => {
        if (mainWindow === window) mainWindow = null;
    });

    window.loadURL(rendererUrl(route));
    mainWindow = window;
};

export const getMainWindow = () => mainWindow;

export const notify = (title: string, body: string, onClick?: () => void) => {
    if (!Notification.isSupported()) return;

    const notification = new Notification({ title, body, icon: iconPath() });
    if (onClick) notification.on('click', onClick);
    notification.show();
};

export const showMainWindow = (route?: string) => {
    if (!mainWindow || mainWindow.isDestroyed()) return createMainWindow(route);

    if (route) mainWindow.webContents.send('app:navigate', route);
    if (mainWindow.isMinimized()) mainWindow.restore();

    mainWindow.show();
    mainWindow.focus();
};

export const setWindowTheme = (isDark: boolean) => {
    mainWindow?.setBackgroundColor(isDark ? DARK : LIGHT);
};

export const broadcast = <K extends keyof IpcEvents>(channel: K, payload: IpcEvents[K]) => {
    for (const window of BrowserWindow.getAllWindows()) {
        if (!window.isDestroyed()) window.webContents.send(channel, payload);
    }
};
