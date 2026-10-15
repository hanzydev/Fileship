import { BrowserWindow, nativeImage, screen } from 'electron';

import { getSettings } from '../store';
import { captureScreen, foregroundWindow, listWindows, type Rect } from './native';

export type Shot = { image: Electron.NativeImage; title?: string };

export const ownWindowHandles = () =>
    BrowserWindow.getAllWindows()
        .filter((window) => !window.isDestroyed())
        .map((window) => {
            const handle = window.getNativeWindowHandle();
            return handle.length >= 8 ? handle.readBigUInt64LE(0) : BigInt(handle.readUInt32LE(0));
        });

export const physicalBounds = (display: Electron.Display): Rect =>
    screen.dipToScreenRect(null, display.bounds);

export const virtualScreen = (): Rect => {
    const rects = screen.getAllDisplays().map(physicalBounds);
    const left = Math.min(...rects.map((rect) => rect.x));
    const top = Math.min(...rects.map((rect) => rect.y));
    const right = Math.max(...rects.map((rect) => rect.x + rect.width));
    const bottom = Math.max(...rects.map((rect) => rect.y + rect.height));

    return { x: left, y: top, width: right - left, height: bottom - top };
};

export const imageFromRgba = (rgba: Uint8Array, width: number, height: number) => {
    const pixels = Buffer.from(rgba.buffer, rgba.byteOffset, rgba.byteLength);

    for (let i = 0; i < pixels.length; i += 4) {
        const red = pixels[i]!;
        pixels[i] = pixels[i + 2]!;
        pixels[i + 2] = red;
    }

    return nativeImage.createFromBitmap(pixels, { width, height });
};

export const toImage = (rect: Rect, data: Buffer) =>
    nativeImage.createFromBitmap(data, { width: rect.width, height: rect.height });

export const captureRect = (rect: Rect): Shot => {
    const { showCursor } = getSettings().capture;
    return { image: toImage(rect, captureScreen(rect, { cursor: showCursor })) };
};

export const captureFullscreen = () => captureRect(virtualScreen());

export const captureActiveMonitor = () =>
    captureRect(physicalBounds(screen.getDisplayNearestPoint(screen.getCursorScreenPoint())));

export const captureActiveWindow = (): Shot | null => {
    const window = foregroundWindow(ownWindowHandles());
    if (!window) return null;

    return { ...captureRect(window), title: window.title };
};

export const snapshotDisplays = () => {
    const { capture } = getSettings();
    const windows = capture.detectWindows
        ? listWindows(ownWindowHandles(), { controls: capture.detectControls })
        : [];

    return screen.getAllDisplays().map((display) => {
        const physical = physicalBounds(display);

        return {
            display,
            physical,
            data: captureScreen(physical, { cursor: false }),
            windows,
        };
    });
};
