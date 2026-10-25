import { app, dialog, ipcMain, nativeTheme } from 'electron';

import {
    HOST_EVENTS,
    INVOKE_CHANNELS,
    type IpcInvoke,
    LOCAL_CHANNELS,
    type Settings,
} from '../shared/ipc';
import { login, logout } from './account';
import { closeDashboard, openDashboard } from './dashboard';
import { invokeHost, onHost } from './host';
import { setLanguage, t } from './i18n';
import {
    broadcast,
    isAppUrl,
    markQuitting,
    setTaskbarProgress,
    setWindowTheme,
    showMainWindow,
} from './windows';

type LocalChannel = (typeof LOCAL_CHANNELS)[number];

type Handlers = {
    [K in LocalChannel]: (
        ...args: Parameters<IpcInvoke[K]>
    ) => ReturnType<IpcInvoke[K]> | Promise<ReturnType<IpcInvoke[K]>>;
};

const pausingHotkeys = new Set<Electron.WebContents>();

const handlers: Handlers = {
    'account:login': (url) => login(url),
    'account:logout': () => logout(),
    'app:openDashboard': (route) => openDashboard(route),
    'window:theme': (isDark) => setWindowTheme(isDark),
    'dialog:pickFolder': async () => {
        const result = await dialog.showOpenDialog({ properties: ['openDirectory'] });
        return result.filePaths[0] ?? null;
    },
    'dialog:pickFfmpeg': async () => {
        const result = await dialog.showOpenDialog({
            title: t('app.chooseFfmpeg'),
            properties: ['openFile'],
            filters: [{ name: 'ffmpeg', extensions: ['exe'] }],
        });

        return result.filePaths[0] ?? null;
    },
};

const isLocal = (channel: string): channel is LocalChannel =>
    (LOCAL_CHANNELS as readonly string[]).includes(channel);

const applyTheme = (settings: Settings) => {
    nativeTheme.themeSource = settings.theme;
};

const watchHotkeyPause = (sender: Electron.WebContents) => {
    if (pausingHotkeys.has(sender)) return;

    pausingHotkeys.add(sender);
    sender.once('destroyed', () => {
        pausingHotkeys.delete(sender);
        invokeHost('hotkeys:pause', false).catch(() => null);
    });
};

const forward = async (event: Electron.IpcMainInvokeEvent, channel: string, args: unknown[]) => {
    if (channel === 'hotkeys:pause' && args[0]) watchHotkeyPause(event.sender);

    const result = await (invokeHost as (...values: unknown[]) => Promise<unknown>)(
        channel,
        ...args,
    );

    if (channel === 'settings:set') applyTheme(result as Settings);
    return result;
};

const listenToHost = () => {
    for (const event of HOST_EVENTS) {
        onHost(event, (payload) => {
            if (event === 'window:show')
                return showMainWindow((payload as { route: string }).route);
            if (event === 'window:dashboard') {
                openDashboard((payload as { route: string }).route);
                return;
            }
            if (event === 'app:quit') {
                markQuitting();
                app.quit();
                return;
            }

            if (event === 'app:language') setLanguage(payload as never);
            if (event === 'account:changed' && !payload) closeDashboard();
            if (event === 'uploads:progress') {
                const items = payload as { progress: number }[];
                const average = items.reduce((sum, item) => sum + item.progress, 0) / items.length;

                setTaskbarProgress(items.length ? average / 100 : -1);
            }

            broadcast(event as never, payload as never);
        });
    }
};

export const loadHostState = async () => {
    const [language, settings] = await Promise.all([
        invokeHost('app:language'),
        invokeHost('settings:get'),
    ]);

    setLanguage(language);
    applyTheme(settings);
};

export const registerIpc = () => {
    listenToHost();

    for (const channel of INVOKE_CHANNELS) {
        ipcMain.handle(channel, (event, ...args) => {
            if (!isAppUrl(event.senderFrame?.url ?? '')) throw new Error('Not allowed');
            if (isLocal(channel)) {
                return (handlers[channel] as (...values: unknown[]) => unknown)(...args);
            }

            return forward(event, channel, args);
        });
    }
};
