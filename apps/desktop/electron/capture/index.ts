import { readFileSync } from 'node:fs';
import { join } from 'node:path';

import { app, BrowserWindow, screen } from 'electron';
import log from 'electron-log/main';

import { native, type PixelOrder, type Rect } from '../native';
import { getSettings } from '../store';

export type Bitmap = { pixels: Buffer; width: number; height: number; order: PixelOrder };

export type Shot = { image: Bitmap; title?: string };

let shutter: Buffer | null = null;

export const playShutter = () => {
    if (!getSettings().capture.playSound) return;

    try {
        shutter ??= readFileSync(join(app.getAppPath(), 'resources', 'shutter.wav'));
        native.playSound(shutter);
    } catch (error) {
        log.warn('Could not play the shutter sound', error);
    }
};

export const bitmapFromImage = (image: Electron.NativeImage): Bitmap | null => {
    if (image.isEmpty()) return null;

    const { width, height } = image.getSize();
    return { pixels: image.toBitmap(), width, height, order: 'bgra' };
};

export const bitmapFromRgba = (rgba: Uint8Array, width: number, height: number): Bitmap => ({
    pixels: Buffer.from(rgba.buffer, rgba.byteOffset, rgba.byteLength),
    width,
    height,
    order: 'rgba',
});

export const encodePng = (bitmap: Bitmap) =>
    native.encodeImage(bitmap.pixels, bitmap.width, bitmap.height, {
        order: bitmap.order,
        format: 'png',
    });

export const encodeThumbnail = async (bitmap: Bitmap) => {
    const jpeg = await native.encodeImage(bitmap.pixels, bitmap.width, bitmap.height, {
        order: bitmap.order,
        format: 'jpeg',
        quality: 75,
        maxWidth: 480,
    });

    return `data:image/jpeg;base64,${jpeg.toString('base64')}`;
};

export const windowHandle = (window: BrowserWindow) => {
    const handle = window.getNativeWindowHandle();
    return handle.length >= 8 ? handle.readBigUInt64LE(0) : BigInt(handle.readUInt32LE(0));
};

export const ownWindowHandles = () =>
    BrowserWindow.getAllWindows()
        .filter((window) => !window.isDestroyed())
        .map(windowHandle);

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

export const captureRect = async (rect: Rect): Promise<Shot> => {
    const { showCursor } = getSettings().capture;
    const [data] = await native.captureRects([rect], { cursor: showCursor });

    playShutter();
    return { image: { pixels: data!, width: rect.width, height: rect.height, order: 'bgra' } };
};

export const captureFullscreen = () => captureRect(virtualScreen());

export const captureActiveMonitor = () =>
    captureRect(physicalBounds(screen.getDisplayNearestPoint(screen.getCursorScreenPoint())));

export const captureActiveWindow = async (): Promise<Shot | null> => {
    const window = native.foregroundWindow(ownWindowHandles());
    if (!window) return null;

    return { ...(await captureRect(window)), title: window.title };
};

export const snapshotDisplays = async () => {
    const { capture } = getSettings();
    const displays = screen.getAllDisplays();
    const rects = displays.map(physicalBounds);

    const [buffers, windows] = await Promise.all([
        native.captureRects(rects, { cursor: false }),
        capture.detectWindows
            ? native.listWindows(ownWindowHandles(), { controls: capture.detectControls })
            : [],
    ]);

    return displays.map((display, index) => ({
        display,
        physical: rects[index]!,
        data: buffers[index]!,
        windows,
    }));
};
