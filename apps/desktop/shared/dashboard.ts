export type DownloadState = 'progressing' | 'completed' | 'cancelled' | 'interrupted';

export type DownloadEntry = {
    id: string;
    name: string;
    state: DownloadState;
    received: number;
    total: number;
    bytesPerSecond: number;
    startedAt: number;
};

export type HarborItemKind = 'file' | 'folder' | 'note';

export type DashboardInvoke = {
    'dashboard:downloads': () => DownloadEntry[];
    'dashboard:openDownload': (id: string) => void;
    'dashboard:showDownload': (id: string) => void;
    'dashboard:cancelDownload': (id: string) => void;
    'dashboard:removeDownload': (id: string) => void;
    'dashboard:clearDownloads': () => void;
    'dashboard:harbor': () => boolean;
    'dashboard:revealInHarbor': (kind: HarborItemKind, id: string) => boolean;
};

export type DashboardEvents = {
    'dashboard:downloadsChanged': DownloadEntry[];
    'dashboard:harborChanged': boolean;
};

export const DASHBOARD_CHANNELS = [
    'dashboard:downloads',
    'dashboard:openDownload',
    'dashboard:showDownload',
    'dashboard:cancelDownload',
    'dashboard:removeDownload',
    'dashboard:clearDownloads',
    'dashboard:harbor',
    'dashboard:revealInHarbor',
] as const satisfies (keyof DashboardInvoke)[];

export type DashboardBridge = {
    version: 1;
    downloads: {
        list: () => Promise<DownloadEntry[]>;
        onChange: (listener: (downloads: DownloadEntry[]) => void) => () => void;
        open: (id: string) => Promise<void>;
        show: (id: string) => Promise<void>;
        cancel: (id: string) => Promise<void>;
        remove: (id: string) => Promise<void>;
        clear: () => Promise<void>;
    };
    harbor: {
        isAvailable: () => Promise<boolean>;
        onChange: (listener: (isAvailable: boolean) => void) => () => void;
        reveal: (kind: HarborItemKind, id: string) => Promise<boolean>;
    };
};
