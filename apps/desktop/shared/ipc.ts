import type { Language } from '../i18n/messages';
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

export type HistoryKind = 'screenshot' | 'recording' | 'gif' | 'file' | 'link';

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

export type OverlayPurpose = 'capture' | 'video' | 'gif' | 'ocr';

export type RecordingKind = 'video' | 'gif';

export type RecorderPhase =
    'idle' | 'preparing' | 'countdown' | 'recording' | 'paused' | 'processing';

export type AudioSourceId = 'microphone' | 'system';

export type AudioTrack = { id: 'mixed' | AudioSourceId; title: string; sources: AudioSourceId[] };

export type RecorderState = {
    phase: RecorderPhase;
    kind: RecordingKind;
    elapsed: number;
    since: number | null;
    countdown: number;
    tracks: AudioTrack[];
    muted: AudioSourceId[];
    maxDuration: number;
};

export type RecorderControl = 'stop' | 'pause' | 'resume' | 'abort';

export type FfmpegStatus = {
    path: string | null;
    version: string | null;
    isManaged: boolean;
    isInstalled: boolean;
    ddagrab: boolean;
    encoders: string[];
    managedVersion: string;
    download: { progress: number; error: string | null } | null;
};

export type ScreenRect = { x: number; y: number; width: number; height: number };

export type OverlayDisplay = {
    id: number;
    rect: ScreenRect;
    scaleFactor: number;
    isPrimary: boolean;
    pixels: Uint8Array;
};

export type OverlayPayload = {
    bounds: ScreenRect;
    displays: OverlayDisplay[];
    windows: (ScreenRect & { title: string; controls: ScreenRect[] })[];
    cursor: { x: number; y: number };
    showMagnifier: boolean;
    purpose: OverlayPurpose;
};

export type OverlayFinish =
    | { type: 'cancel' }
    | { type: 'area'; rect: ScreenRect }
    | { type: 'region'; rect: ScreenRect; width: number; height: number; rgba: Uint8Array };

export type Folder = { id: string; name: string };

export type HarborStorage = { used: number; limit: number | null };

export type HarborStatus = {
    state: 'off' | 'starting' | 'syncing' | 'ready' | 'paused' | 'error';
    root: string;
    files: number;
    folders: number;
    error: string | null;
    storage: HarborStorage | null;
    localSize: number | null;
    lastSyncedAt: number | null;
};

export type HarborActivityKind =
    | 'upload'
    | 'update'
    | 'note'
    | 'folder'
    | 'rename'
    | 'move'
    | 'trash'
    | 'delete'
    | 'freeUp'
    | 'error';

export type HarborActivity = {
    id: string;
    kind: HarborActivityKind;
    name: string;
    detail: string | null;
    at: number;
};

export type HarborBreakdown = { type: string; size: number };

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
    'app:language': () => Language;
    'app:openDashboard': (route?: string) => void;
    'window:theme': (isDark: boolean) => void;
    'updates:get': () => UpdateStatus;
    'updates:check': () => void;
    'updates:install': () => void;
    'capture:run': (action: HotkeyAction) => void;
    'overlay:loaded': () => void;
    'overlay:ready': () => void;
    'overlay:finish': (result: OverlayFinish) => void;
    'history:list': (offset: number, limit: number) => { items: HistoryItem[]; total: number };
    'history:copy': (id: string) => void;
    'history:open': (id: string) => void;
    'history:showFile': (id: string) => void;
    'history:showInHarbor': (id: string) => void;
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
    'dialog:pickFfmpeg': () => string | null;
    'ffmpeg:status': () => FfmpegStatus;
    'ffmpeg:download': () => void;
    'recording:preview': () => string;
    'recorder:get': () => RecorderState;
    'recorder:ready': (error: string | null) => void;
    'recorder:audio': (track: number, chunk: ArrayBuffer) => void;
    'recorder:control': (action: RecorderControl) => void;
    'recorder:mute': (source: AudioSourceId, muted: boolean) => void;
    'recorder:visible': (view: 'toolbar' | 'border') => void;
    'harbor:status': () => HarborStatus;
    'harbor:open': () => void;
    'harbor:setFolder': (folder: string) => HarborStatus;
    'harbor:pause': (paused: boolean) => HarborStatus;
    'harbor:freeUpSpace': () => number;
    'harbor:activity': () => HarborActivity[];
    'harbor:breakdown': () => HarborBreakdown[];
};

export type IpcEvents = {
    'app:navigate': string;
    'app:language': Language;
    'account:changed': Account | null;
    'updates:status': UpdateStatus;
    'overlay:open': OverlayPayload;
    'overlay:close': null;
    'history:changed': null;
    'uploads:progress': UploadProgress[];
    'ffmpeg:status': FfmpegStatus;
    'recorder:state': RecorderState;
    'harbor:status': HarborStatus;
    'harbor:activity': HarborActivity[];
};

export const INVOKE_CHANNELS = [
    'instance:check',
    'account:get',
    'account:login',
    'account:logout',
    'settings:get',
    'settings:set',
    'app:info',
    'app:language',
    'app:openDashboard',
    'window:theme',
    'updates:get',
    'updates:check',
    'updates:install',
    'capture:run',
    'overlay:loaded',
    'overlay:ready',
    'overlay:finish',
    'history:list',
    'history:copy',
    'history:open',
    'history:showFile',
    'history:showInHarbor',
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
    'dialog:pickFfmpeg',
    'ffmpeg:status',
    'ffmpeg:download',
    'recording:preview',
    'recorder:get',
    'recorder:ready',
    'recorder:audio',
    'recorder:control',
    'recorder:mute',
    'recorder:visible',
    'harbor:status',
    'harbor:open',
    'harbor:setFolder',
    'harbor:pause',
    'harbor:freeUpSpace',
    'harbor:activity',
    'harbor:breakdown',
] as const satisfies (keyof IpcInvoke)[];

export const EVENT_CHANNELS = [
    'app:navigate',
    'app:language',
    'account:changed',
    'updates:status',
    'overlay:open',
    'overlay:close',
    'history:changed',
    'uploads:progress',
    'ffmpeg:status',
    'recorder:state',
    'harbor:status',
    'harbor:activity',
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
