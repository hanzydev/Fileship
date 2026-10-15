import { app, BrowserWindow, session, shell } from 'electron';

import { desktopUserAgent } from './api';
import { getStore, getToken } from './store';
import { iconPath, themeBackground } from './windows';

const PARTITION = 'persist:dashboard';

let dashboardWindow: BrowserWindow | null = null;

const dashboardSession = () => session.fromPartition(PARTITION);

const signIn = async (instanceUrl: string, token: string) => {
    const windowSession = dashboardSession();

    windowSession.setUserAgent(`${app.userAgentFallback} ${desktopUserAgent()}`);

    await windowSession.cookies.set({
        url: instanceUrl,
        name: 'token',
        value: token,
        httpOnly: true,
        secure: instanceUrl.startsWith('https://'),
        sameSite: 'lax',
        expirationDate: Math.floor(Date.now() / 1000) + 400 * 24 * 60 * 60,
    });

    return windowSession;
};

export const openDashboard = async (route = '/dashboard') => {
    const { instanceUrl, siteName } = getStore();
    const token = getToken();
    if (!instanceUrl || !token) return;

    if (dashboardWindow && !dashboardWindow.isDestroyed()) {
        if (route !== '/dashboard') dashboardWindow.loadURL(`${instanceUrl}${route}`);
        if (dashboardWindow.isMinimized()) dashboardWindow.restore();
        dashboardWindow.focus();
        return;
    }

    const windowSession = await signIn(instanceUrl, token);

    dashboardWindow = new BrowserWindow({
        width: 1280,
        height: 820,
        minWidth: 760,
        minHeight: 520,
        title: siteName,
        icon: iconPath(),
        backgroundColor: themeBackground(),
        autoHideMenuBar: true,
        webPreferences: {
            session: windowSession,
            contextIsolation: true,
            sandbox: true,
            nodeIntegration: false,
        },
    });

    dashboardWindow.webContents.setWindowOpenHandler(({ url }) => {
        if (/^https?:\/\//.test(url)) shell.openExternal(url);
        return { action: 'deny' };
    });

    dashboardWindow.webContents.on('will-navigate', (event, url) => {
        if (new URL(url).origin === instanceUrl) return;

        event.preventDefault();
        if (/^https?:\/\//.test(url)) shell.openExternal(url);
    });

    dashboardWindow.on('closed', () => {
        dashboardWindow = null;
    });

    dashboardWindow.loadURL(`${instanceUrl}${route}`);
};

export const closeDashboard = async () => {
    if (dashboardWindow && !dashboardWindow.isDestroyed()) dashboardWindow.destroy();
    await dashboardSession().clearStorageData();
};
