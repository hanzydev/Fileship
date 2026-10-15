import { contextBridge, ipcRenderer, webUtils } from 'electron';

import { type DesktopBridge, EVENT_CHANNELS, INVOKE_CHANNELS } from '../shared/ipc';

const invokeChannels = new Set<string>(INVOKE_CHANNELS);
const eventChannels = new Set<string>(EVENT_CHANNELS);

const bridge: DesktopBridge = {
    invoke: (channel, ...args) => {
        if (!invokeChannels.has(channel)) throw new Error(`Unknown channel ${channel}`);
        return ipcRenderer.invoke(channel, ...args);
    },
    on: (channel, listener) => {
        if (!eventChannels.has(channel)) throw new Error(`Unknown channel ${channel}`);

        const handleEvent = (_: Electron.IpcRendererEvent, payload: any) => listener(payload);
        ipcRenderer.on(channel, handleEvent);

        return () => ipcRenderer.off(channel, handleEvent);
    },
    pathForFile: (file) => webUtils.getPathForFile(file),
};

contextBridge.exposeInMainWorld('fileship', bridge);
