export type DesktopDownloadState = 'progressing' | 'completed' | 'cancelled' | 'interrupted';

export type DesktopDownload = {
    id: string;
    name: string;
    state: DesktopDownloadState;
    received: number;
    total: number;
    bytesPerSecond: number;
    startedAt: number;
};

export type HarborItemKind = 'file' | 'folder' | 'note';

export type DesktopBridge = {
    version: number;
    downloads: {
        list: () => Promise<DesktopDownload[]>;
        onChange: (listener: (downloads: DesktopDownload[]) => void) => () => void;
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

declare global {
    interface Window {
        fileshipDesktop?: DesktopBridge;
    }
}
