import { app, ipcMain, nativeTheme, shell } from 'electron';

import { INVOKE_CHANNELS, type IpcInvoke } from '../shared/ipc';
import { checkInstance, getAccount, login, logout } from './account';
import { closeDashboard, openDashboard } from './dashboard';
import { getSettings, updateSettings } from './store';
import { refreshTray } from './tray';
import { checkForUpdates, getUpdateStatus, installUpdate } from './updater';
import { isAppUrl, setWindowTheme } from './windows';

type Handlers = {
    [K in keyof IpcInvoke]: (
        ...args: Parameters<IpcInvoke[K]>
    ) => ReturnType<IpcInvoke[K]> | Promise<ReturnType<IpcInvoke[K]>>;
};

const applySettings = () => {
    const { startAtLogin, theme } = getSettings();

    if (app.isPackaged) app.setLoginItemSettings({ openAtLogin: startAtLogin, args: ['--hidden'] });
    nativeTheme.themeSource = theme;
};

const handlers: Handlers = {
    'instance:check': (url) => checkInstance(url),
    'account:get': async () => {
        const account = await getAccount();
        refreshTray();
        return account;
    },
    'account:login': async (url) => {
        const account = await login(url);
        refreshTray();
        return account;
    },
    'account:logout': async () => {
        await closeDashboard();
        await logout();
        refreshTray();
    },
    'settings:get': () => getSettings(),
    'settings:set': (patch) => {
        const settings = updateSettings(patch);
        applySettings();
        return settings;
    },
    'app:info': () => ({ version: app.getVersion(), isPackaged: app.isPackaged }),
    'app:openExternal': (url) => {
        if (/^https?:\/\//.test(url)) shell.openExternal(url);
    },
    'app:openDashboard': (route) => openDashboard(route),
    'window:theme': (isDark) => setWindowTheme(isDark),
    'updates:get': () => getUpdateStatus(),
    'updates:check': () => {
        checkForUpdates();
    },
    'updates:install': () => installUpdate(),
};

export const registerIpc = () => {
    applySettings();

    for (const channel of INVOKE_CHANNELS) {
        ipcMain.handle(channel, (event, ...args) => {
            if (!isAppUrl(event.senderFrame?.url ?? '')) throw new Error('Not allowed');
            return (handlers[channel] as (...values: unknown[]) => unknown)(...args);
        });
    }
};
