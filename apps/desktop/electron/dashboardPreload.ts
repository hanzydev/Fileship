import { contextBridge, ipcRenderer } from 'electron';

import type { DashboardBridge, DashboardEvents, DashboardInvoke } from '../shared/dashboard';

const invoke =
    <K extends keyof DashboardInvoke>(channel: K) =>
    (...args: Parameters<DashboardInvoke[K]>) =>
        ipcRenderer.invoke(channel, ...args) as Promise<ReturnType<DashboardInvoke[K]>>;

const listen =
    <K extends keyof DashboardEvents>(channel: K) =>
    (listener: (payload: DashboardEvents[K]) => void) => {
        const handleEvent = (_: Electron.IpcRendererEvent, payload: DashboardEvents[K]) =>
            listener(payload);

        ipcRenderer.on(channel, handleEvent);
        return () => {
            ipcRenderer.off(channel, handleEvent);
        };
    };

const bridge: DashboardBridge = {
    version: 1,
    downloads: {
        list: invoke('dashboard:downloads'),
        onChange: listen('dashboard:downloadsChanged'),
        open: invoke('dashboard:openDownload'),
        show: invoke('dashboard:showDownload'),
        cancel: invoke('dashboard:cancelDownload'),
        remove: invoke('dashboard:removeDownload'),
        clear: invoke('dashboard:clearDownloads'),
    },
    harbor: {
        isAvailable: invoke('dashboard:harbor'),
        onChange: listen('dashboard:harborChanged'),
        reveal: invoke('dashboard:revealInHarbor'),
    },
};

contextBridge.exposeInMainWorld('fileshipDesktop', bridge);
