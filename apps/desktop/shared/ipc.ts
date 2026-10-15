export const MIN_FILESHIP_VERSION = '6.0.0';

export type Permission = 'Admin' | 'UploadFiles' | 'TakeNotes' | 'ShortenLinks';

export type AccountUser = {
    id: string;
    username: string;
    avatar: string | null;
    permissions: Permission[];
    superAdmin: boolean;
};

export type Account = {
    instanceUrl: string;
    siteName: string;
    user: AccountUser;
};

export type InstanceInfo = {
    url: string;
    version: string;
    siteName: string;
};

export type Theme = 'system' | 'light' | 'dark';

export type Settings = {
    startAtLogin: boolean;
    theme: Theme;
};

export type UpdateState =
    'idle' | 'checking' | 'latest' | 'downloading' | 'ready' | 'error' | 'unsupported';

export type UpdateStatus = {
    state: UpdateState;
    version?: string;
    progress?: number;
    error?: string;
};

export type AppInfo = {
    version: string;
    isPackaged: boolean;
};

export type IpcInvoke = {
    'instance:check': (url: string) => InstanceInfo;
    'account:get': () => Account | null;
    'account:login': (url: string) => Account;
    'account:logout': () => void;
    'settings:get': () => Settings;
    'settings:set': (patch: Partial<Settings>) => Settings;
    'app:info': () => AppInfo;
    'app:openExternal': (url: string) => void;
    'app:openDashboard': (route?: string) => void;
    'window:theme': (isDark: boolean) => void;
    'updates:get': () => UpdateStatus;
    'updates:check': () => void;
    'updates:install': () => void;
};

export type IpcEvents = {
    'app:navigate': string;
    'account:changed': Account | null;
    'updates:status': UpdateStatus;
};

export const INVOKE_CHANNELS = [
    'instance:check',
    'account:get',
    'account:login',
    'account:logout',
    'settings:get',
    'settings:set',
    'app:info',
    'app:openExternal',
    'app:openDashboard',
    'window:theme',
    'updates:get',
    'updates:check',
    'updates:install',
] as const satisfies (keyof IpcInvoke)[];

export const EVENT_CHANNELS = [
    'app:navigate',
    'account:changed',
    'updates:status',
] as const satisfies (keyof IpcEvents)[];

export type DesktopBridge = {
    invoke: <K extends keyof IpcInvoke>(
        channel: K,
        ...args: Parameters<IpcInvoke[K]>
    ) => Promise<ReturnType<IpcInvoke[K]>>;
    on: <K extends keyof IpcEvents>(
        channel: K,
        listener: (payload: IpcEvents[K]) => void,
    ) => () => void;
};
