import type { HotkeyAction, Settings } from './settings';

export type { Settings, Theme } from './settings';

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

export type UpdateState =
    'idle' | 'checking' | 'latest' | 'downloading' | 'ready' | 'error' | 'unsupported';

export type UpdateStatus = {
    state: UpdateState;
    version?: string;
    progress?: number;
    error?: string;
};

export type HistoryKind = 'screenshot' | 'recording' | 'gif' | 'file';

export type HistoryItem = {
    id: string;
    kind: HistoryKind;
    name: string;
    localPath: string | null;
    url: string | null;
    shortUrl: string | null;
    fileId: string | null;
    size: number;
    thumbnail: string | null;
    error: string | null;
    createdAt: number;
};

export type UploadProgress = {
    id: string;
    name: string;
    progress: number;
};

export type HotkeyStatus = Partial<Record<HotkeyAction, 'ok' | 'conflict' | 'invalid'>>;

export type LocalRect = { x: number; y: number; width: number; height: number };

export type OverlayPayload = {
    displayId: number;
    index: number;
    width: number;
    height: number;
    scaleFactor: number;
    pixels: Uint8Array;
    windows: (LocalRect & { title: string; controls: LocalRect[] })[];
    cursor: { x: number; y: number };
    showMagnifier: boolean;
    quickCrop: boolean;
    isActive: boolean;
};

export type OverlayFinish =
    | { type: 'cancel' }
    | { type: 'fullscreen' }
    | { type: 'monitor'; index: number }
    | {
          type: 'region';
          displayId: number;
          region: LocalRect;
          width: number;
          height: number;
          rgba: Uint8Array;
      };

export type Folder = { id: string; name: string };

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
    'capture:run': (action: HotkeyAction) => void;
    'overlay:ready': (displayId: number) => void;
    'overlay:finish': (result: OverlayFinish) => void;
    'history:list': (offset: number, limit: number) => { items: HistoryItem[]; total: number };
    'history:copy': (id: string) => void;
    'history:open': (id: string) => void;
    'history:showFile': (id: string) => void;
    'history:remove': (id: string) => void;
    'history:clear': () => void;
    'uploads:paths': (paths: string[]) => void;
    'uploads:progress': () => UploadProgress[];
    'hotkeys:status': () => HotkeyStatus;
    'hotkeys:pause': (paused: boolean) => HotkeyStatus;
    'hotkeys:snippingTool': () => boolean;
    'hotkeys:setSnippingTool': (enabled: boolean) => HotkeyStatus;
    'folders:list': () => Folder[];
    'dialog:pickFolder': () => string | null;
};

export type IpcEvents = {
    'app:navigate': string;
    'account:changed': Account | null;
    'updates:status': UpdateStatus;
    'overlay:open': OverlayPayload;
    'overlay:close': null;
    'history:changed': null;
    'uploads:progress': UploadProgress[];
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
    'capture:run',
    'overlay:ready',
    'overlay:finish',
    'history:list',
    'history:copy',
    'history:open',
    'history:showFile',
    'history:remove',
    'history:clear',
    'uploads:paths',
    'uploads:progress',
    'hotkeys:status',
    'hotkeys:pause',
    'hotkeys:snippingTool',
    'hotkeys:setSnippingTool',
    'folders:list',
    'dialog:pickFolder',
] as const satisfies (keyof IpcInvoke)[];

export const EVENT_CHANNELS = [
    'app:navigate',
    'account:changed',
    'updates:status',
    'overlay:open',
    'overlay:close',
    'history:changed',
    'uploads:progress',
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
    pathForFile: (file: File) => string;
};
