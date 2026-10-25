import { app, BrowserWindow, session } from 'electron';

import type { Account } from '../shared/ipc';
import { closeDashboard } from './dashboard';
import { invokeHost } from './host';
import { getLanguage, t } from './i18n';
import {
    desktopUserAgent,
    getMainWindow,
    iconPath,
    openWebUrl,
    restrictPermissions,
    themeBackground,
    webPreferences,
} from './windows';

const readToken = (windowSession: Electron.Session, instanceUrl: string) =>
    windowSession.cookies
        .get({ url: instanceUrl, name: 'token' })
        .then((cookies) => cookies[0]?.value ?? null);

export const login = async (input: string): Promise<Account> => {
    const instance = await invokeHost('instance:check', input);
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

    return invokeHost('account:setSession', { instanceUrl: instance.url, token });
};

export const logout = async () => {
    await closeDashboard();
    await invokeHost('account:clearSession');
};
