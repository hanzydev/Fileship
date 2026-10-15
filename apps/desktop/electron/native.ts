import { join } from 'node:path';

import { app } from 'electron';

export type Rect = { x: number; y: number; width: number; height: number };

export type WindowRect = Rect & { title: string; controls: Rect[] };

export type DisplayOutput = { adapter: number; output: number; rect: Rect };

export type PixelOrder = 'bgra' | 'rgba';

export type EncodeOptions = {
    order: PixelOrder;
    format: 'png' | 'jpeg';
    quality?: number;
    maxWidth?: number;
};

type NativeCore = {
    captureRects: (rects: Rect[], options: { cursor?: boolean }) => Promise<Buffer[]>;
    listWindows: (ignored: bigint[], options: { controls?: boolean }) => Promise<WindowRect[]>;
    foregroundWindow: (ignored: bigint[]) => (Rect & { title: string }) | null;
    forceForeground: (handle: bigint) => void;
    listOutputs: () => DisplayOutput[];
    encodeImage: (
        pixels: Buffer,
        width: number,
        height: number,
        options: EncodeOptions,
    ) => Promise<Buffer>;
};

const addonPath = () =>
    app.isPackaged
        ? join(process.resourcesPath, 'native', 'fileship-native.node')
        : join(app.getAppPath(), 'build', 'native', 'fileship-native.node');

const load = () => {
    const addon = { exports: {} as NativeCore };
    process.dlopen(addon, addonPath());
    return addon.exports;
};

export const native = load();
