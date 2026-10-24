import { app, BrowserWindow, session } from 'electron';

import { getAccount } from './account';
import { desktopUserAgent } from './api';
import { getLanguage } from './i18n';
import { getStore, getToken } from './store';
import {
    iconPath,
    openWebUrl,
    restrictPermissions,
    themeBackground,
    webPreferences,
} from './windows';

const PARTITION = 'persist:dashboard';

let dashboardWindow: BrowserWindow | null = null;
let opening: Promise<void> | null = null;

const dashboardSession = () => session.fromPartition(PARTITION);

const signIn = async (instanceUrl: string, token: string) => {
    const windowSession = dashboardSession();

    windowSession.setUserAgent(`${app.userAgentFallback} ${desktopUserAgent()}`, getLanguage());
    restrictPermissions(windowSession);

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

const createDashboard = async (url: string, siteName: string, token: string) => {
    const { origin } = new URL(url);
    const windowSession = await signIn(origin, token);

    const window = new BrowserWindow({
        width: 1280,
        height: 820,
        minWidth: 760,
        minHeight: 520,
        title: siteName,
        icon: iconPath(),
        backgroundColor: themeBackground(),
        autoHideMenuBar: true,
        webPreferences: webPreferences({ session: windowSession }),
    });

    window.webContents.setWindowOpenHandler(({ url: target }) => {
        openWebUrl(target);
        return { action: 'deny' };
    });

    window.webContents.on('will-navigate', (event, target) => {
        if (new URL(target).origin === origin) return;

        event.preventDefault();
        openWebUrl(target);
    });

    const handleNavigate = (_: unknown, target: string) => {
        if (new URL(target).pathname.startsWith('/login')) getAccount();
    };

    window.webContents.on('did-navigate', handleNavigate);
    window.webContents.on('did-navigate-in-page', handleNavigate);

    window.on('closed', () => {
        if (dashboardWindow === window) dashboardWindow = null;
    });

    dashboardWindow = window;
    window.loadURL(url);
};

export const openDashboard = async (route = '/dashboard') => {
    const { instanceUrl, siteName } = getStore();
    const token = getToken();
    if (!instanceUrl || !token) return;

    if (!dashboardWindow || dashboardWindow.isDestroyed()) {
        if (!opening) {
            opening = createDashboard(`${instanceUrl}${route}`, siteName, token).finally(() => {
                opening = null;
            });

            return opening;
        }

        await opening;
    }

    const window = dashboardWindow;
    if (!window || window.isDestroyed()) return;

    if (route !== '/dashboard') window.loadURL(`${instanceUrl}${route}`);
    if (window.isMinimized()) window.restore();
    window.focus();
};

export const closeDashboard = async () => {
    if (dashboardWindow && !dashboardWindow.isDestroyed()) dashboardWindow.destroy();
    await dashboardSession().clearStorageData();
};
