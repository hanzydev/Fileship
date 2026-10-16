import { app, BrowserWindow, session } from 'electron';
import log from 'electron-log/main';
import semver from 'semver';

import {
    type Account,
    type AccountUser,
    type InstanceInfo,
    MIN_FILESHIP_VERSION,
} from '../shared/ipc';
import { ApiError, desktopUserAgent, normalizeInstanceUrl, request } from './api';
import { stopDrive } from './drive';
import { getLanguage, t } from './i18n';
import { clearSession, getStore, getToken, setSession } from './store';
import { refreshTray } from './tray';
import {
    broadcast,
    getMainWindow,
    iconPath,
    openWebUrl,
    restrictPermissions,
    themeBackground,
    webPreferences,
} from './windows';

const CHECK_REUSE_MS = 5 * 60 * 1000;

let lastCheck: { info: InstanceInfo; at: number } | null = null;

export const checkInstance = async (input: string): Promise<InstanceInfo> => {
    let url: string;

    try {
        url = normalizeInstanceUrl(input);
    } catch {
        throw new Error(t('app.invalidAddress'));
    }

    const health = await request<{ version?: string }>(url, '/api/healthz', {
        timeout: 10_000,
    }).catch((error: ApiError) => {
        throw new Error(
            error.status === 503 ? t('app.serverStarting') : t('app.serverNotFound', { url }),
        );
    });

    const version = semver.coerce(health?.version)?.version;
    if (!version) throw new Error(t('app.serverNotFound', { url }));

    if (semver.lt(version, MIN_FILESHIP_VERSION)) {
        throw new Error(t('app.serverTooOld', { version, minVersion: MIN_FILESHIP_VERSION }));
    }

    const settings = await request<{ siteName?: string }>(url, '/api/settings/public').catch(
        () => null,
    );

    const info = { url, version, siteName: settings?.siteName || 'Fileship' };
    lastCheck = { info, at: Date.now() };

    return info;
};

const checkedInstance = (input: string) => {
    const isRecent = lastCheck?.info.url === input && Date.now() - lastCheck.at < CHECK_REUSE_MS;

    return isRecent ? lastCheck!.info : checkInstance(input);
};

const fetchUser = (instanceUrl: string, token: string) =>
    request<AccountUser>(instanceUrl, '/api/users/@me', { token });

const toAccount = (): Account | null => {
    const { instanceUrl, siteName, user } = getStore();
    return instanceUrl && user ? { instanceUrl, siteName, user } : null;
};

export const getAccount = async (): Promise<Account | null> => {
    const { instanceUrl } = getStore();
    const token = getToken();

    if (!instanceUrl || !token) return null;

    try {
        const user = await fetchUser(instanceUrl, token);
        setSession({ ...getStore(), instanceUrl, token, user });
    } catch (error) {
        if (error instanceof ApiError && error.status === 401) {
            handleUnauthorized();
            return null;
        }

        log.warn('Could not refresh the account, using the cached one', error);
    }

    return toAccount();
};

const readToken = (windowSession: Electron.Session, instanceUrl: string) =>
    windowSession.cookies
        .get({ url: instanceUrl, name: 'token' })
        .then((cookies) => cookies[0]?.value ?? null);

export const login = async (input: string): Promise<Account> => {
    const instance = await checkedInstance(input);
    const windowSession = session.fromPartition(`login-${Date.now()}`);

    windowSession.setUserAgent(`${app.userAgentFallback} ${desktopUserAgent()}`, getLanguage());
    restrictPermissions(windowSession);

    const loginWindow = new BrowserWindow({
        width: 480,
        height: 720,
        parent: getMainWindow() ?? undefined,
        modal: !!getMainWindow(),
        title: t('app.loginTitle', { siteName: instance.siteName }),
        icon: iconPath(),
        backgroundColor: themeBackground(),
        autoHideMenuBar: true,
        webPreferences: webPreferences({ session: windowSession }),
    });

    loginWindow.webContents.setWindowOpenHandler(({ url }) => {
        openWebUrl(url);
        return { action: 'deny' };
    });

    const token = await new Promise<string>((resolve, reject) => {
        let isDone = false;

        const handleNavigate = async (_: unknown, url: string) => {
            const { origin, pathname } = new URL(url);
            if (origin !== instance.url || /^\/(login|register)/.test(pathname)) return;

            const value = await readToken(windowSession, instance.url);
            if (!value || isDone) return;

            isDone = true;
            resolve(value);
            loginWindow.close();
        };

        loginWindow.webContents.on('did-navigate', handleNavigate);
        loginWindow.webContents.on('did-navigate-in-page', handleNavigate);

        loginWindow.on('closed', () => {
            if (!isDone) reject(new Error(t('app.loginCancelled')));
        });

        loginWindow.loadURL(`${instance.url}/login`);
    }).finally(() => windowSession.clearStorageData());

    const user = await fetchUser(instance.url, token);

    if (
        !user.superAdmin &&
        !['Admin', 'UploadFiles', 'ShortenLinks'].some((permission) =>
            user.permissions.includes(permission as never),
        )
    ) {
        await request(instance.url, '/api/auth/logout', { method: 'POST', token }).catch(
            () => null,
        );

        throw new Error(t('app.noUploadPermission'));
    }

    setSession({ instanceUrl: instance.url, siteName: instance.siteName, token, user });

    const account = toAccount()!;
    broadcast('account:changed', account);

    return account;
};

export const logout = async () => {
    const { instanceUrl } = getStore();
    const token = getToken();

    if (instanceUrl && token) {
        await request(instanceUrl, '/api/auth/logout', { method: 'POST', token }).catch(() => null);
    }

    clearSession();
    broadcast('account:changed', null);
    refreshTray();
};

export const handleUnauthorized = () => {
    stopDrive({ remove: true, forget: true });
    clearSession();
    broadcast('account:changed', null);
};
