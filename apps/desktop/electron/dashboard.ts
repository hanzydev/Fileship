import { app, BrowserWindow, session } from 'electron';

import { invokeHost } from './host';
import { getLanguage } from './i18n';
import {
    desktopUserAgent,
    iconPath,
    openWebUrl,
    restrictPermissions,
    themeBackground,
    webPreferences,
} from './windows';

const PARTITION = 'persist:dashboard';
const SSO_CALLBACK = '/api/auth/oidc/callback';
const SSO_VERIFY = '/api/auth/oidc/verify';

let dashboardWindow: BrowserWindow | null = null;
let opening: Promise<void> | null = null;

const dashboardSession = () => session.fromPartition(PARTITION);

const originOf = (url: string) => URL.parse(url)?.origin;

const startsSignIn = (url: string, origin: string) => {
    const redirectUri = URL.parse(URL.parse(url)?.searchParams.get('redirect_uri') ?? '');
    return redirectUri?.origin === origin && redirectUri.pathname === SSO_CALLBACK;
};

const denyWindow = ({ url }: Electron.HandlerDetails) => {
    openWebUrl(url);
    return { action: 'deny' } as const;
};

const keepOnInstance = (contents: Electron.WebContents, origin: string) => {
    let isSigningIn = false;

    contents.on('will-navigate', (event, target) => {
        if (originOf(target) === origin) return;

        if (startsSignIn(target, origin)) {
            isSigningIn = true;
            return;
        }

        if (isSigningIn && originOf(contents.getURL()) !== origin) return;

        event.preventDefault();
        openWebUrl(target);
    });

    contents.on('will-redirect', ({ url, isMainFrame }) => {
        if (isMainFrame && startsSignIn(url, origin)) isSigningIn = true;
    });

    contents.on('did-navigate', (_, target) => {
        if (originOf(target) === origin) isSigningIn = false;
    });
};

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

    window.webContents.setWindowOpenHandler((details) => {
        if (details.url !== `${origin}${SSO_VERIFY}`) return denyWindow(details);

        return {
            action: 'allow',
            overrideBrowserWindowOptions: {
                parent: window,
                icon: iconPath(),
                backgroundColor: themeBackground(),
                autoHideMenuBar: true,
                webPreferences: webPreferences(),
            },
        };
    });

    window.webContents.on('did-create-window', (child) => {
        child.webContents.setWindowOpenHandler(denyWindow);
        keepOnInstance(child.webContents, origin);
    });

    keepOnInstance(window.webContents, origin);

    const handleNavigate = (_: unknown, target: string) => {
        if (new URL(target).pathname.startsWith('/login'))
            invokeHost('account:get').catch(() => null);
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
    const [account, token] = await Promise.all([
        invokeHost('account:get'),
        invokeHost('account:token'),
    ]);
    if (!account || !token) return;

    if (!dashboardWindow || dashboardWindow.isDestroyed()) {
        if (!opening) {
            opening = createDashboard(
                `${account.instanceUrl}${route}`,
                account.siteName,
                token,
            ).finally(() => {
                opening = null;
            });

            return opening;
        }

        await opening;
    }

    const window = dashboardWindow;
    if (!window || window.isDestroyed()) return;

    if (route !== '/dashboard') window.loadURL(`${account.instanceUrl}${route}`);
    if (window.isMinimized()) window.restore();
    window.focus();
};

export const closeDashboard = async () => {
    if (dashboardWindow && !dashboardWindow.isDestroyed()) dashboardWindow.destroy();
    await dashboardSession().clearStorageData();
};
