import { join } from 'node:path';

import { app, BrowserWindow, ipcMain, session, shell } from 'electron';

import {
    DASHBOARD_CHANNELS,
    type DashboardEvents,
    type DashboardInvoke,
    type HarborItemKind,
} from '../shared/dashboard';
import type { HarborStatus } from '../shared/ipc';
import {
    cancelDownload,
    cancelDownloads,
    clearDownloads,
    downloadProgress,
    listDownloads,
    openDownload,
    removeDownload,
    showDownload,
    trackDownloads,
    watchDownloads,
} from './downloads';
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
const HARBOR_KINDS = new Set<HarborItemKind>(['file', 'folder', 'note']);
const HARBOR_STATES = new Set<HarborStatus['state']>(['ready', 'syncing', 'paused']);
const ITEM_ID = /^[\w-]{1,64}$/;

type DashboardHandlers = {
    [K in keyof DashboardInvoke]: (
        ...args: Parameters<DashboardInvoke[K]>
    ) => ReturnType<DashboardInvoke[K]> | Promise<ReturnType<DashboardInvoke[K]>>;
};

let dashboardWindow: BrowserWindow | null = null;
let opening: Promise<void> | null = null;
let instanceOrigin: string | null = null;
let isListening = false;
let isHarborAvailable = false;

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
    trackDownloads(windowSession);

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
        webPreferences: webPreferences({
            session: windowSession,
            preload: join(__dirname, 'dashboardPreload.cjs'),
        }),
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

    window.webContents.on('did-navigate', (event, target) => {
        isListening = false;
        handleNavigate(event, target);
    });
    window.webContents.on('did-navigate-in-page', handleNavigate);

    window.on('closed', () => {
        if (dashboardWindow !== window) return;

        dashboardWindow = null;
        isListening = false;
    });

    dashboardWindow = window;
    instanceOrigin = origin;
    window.setProgressBar(downloadProgress());
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
    cancelDownloads();
    if (dashboardWindow && !dashboardWindow.isDestroyed()) dashboardWindow.destroy();
    await dashboardSession().clearStorageData();
};

const sendToDashboard = <K extends keyof DashboardEvents>(
    channel: K,
    payload: DashboardEvents[K],
) => {
    if (dashboardWindow && !dashboardWindow.isDestroyed()) {
        dashboardWindow.webContents.send(channel, payload);
    }
};

const isDashboardFrame = ({ sender, senderFrame }: Electron.IpcMainInvokeEvent) =>
    !!dashboardWindow &&
    !dashboardWindow.isDestroyed() &&
    sender === dashboardWindow.webContents &&
    senderFrame === sender.mainFrame &&
    originOf(senderFrame.url) === instanceOrigin;

export const updateDashboardHarbor = (status: HarborStatus) => {
    const isAvailable = HARBOR_STATES.has(status.state);
    if (isAvailable === isHarborAvailable) return;

    isHarborAvailable = isAvailable;
    sendToDashboard('dashboard:harborChanged', isAvailable);
};

const revealInHarbor = async (kind: HarborItemKind, id: string) => {
    if (!HARBOR_KINDS.has(kind) || typeof id !== 'string' || !ITEM_ID.test(id)) return false;

    const path = await invokeHost('harbor:locate', kind, id);
    if (path) shell.showItemInFolder(path);

    return !!path;
};

const dashboardHandlers: DashboardHandlers = {
    'dashboard:downloads': () => {
        isListening = true;
        return listDownloads();
    },
    'dashboard:openDownload': (id) => openDownload(id),
    'dashboard:showDownload': (id) => showDownload(id),
    'dashboard:cancelDownload': (id) => cancelDownload(id),
    'dashboard:removeDownload': (id) => removeDownload(id),
    'dashboard:clearDownloads': () => clearDownloads(),
    'dashboard:harbor': async () => {
        updateDashboardHarbor(await invokeHost('harbor:status'));
        return isHarborAvailable;
    },
    'dashboard:revealInHarbor': (kind, id) => revealInHarbor(kind, id),
};

export const registerDashboardIpc = () => {
    watchDownloads({
        onChange: (entries) => {
            sendToDashboard('dashboard:downloadsChanged', entries);
            if (dashboardWindow && !dashboardWindow.isDestroyed()) {
                dashboardWindow.setProgressBar(downloadProgress());
            }
        },
        isWatched: () =>
            isListening &&
            !!dashboardWindow &&
            !dashboardWindow.isDestroyed() &&
            dashboardWindow.isFocused(),
    });

    for (const channel of DASHBOARD_CHANNELS) {
        ipcMain.handle(channel, (event, ...args) => {
            if (!isDashboardFrame(event)) throw new Error('Not allowed');
            return (dashboardHandlers[channel] as (...values: unknown[]) => unknown)(...args);
        });
    }
};
