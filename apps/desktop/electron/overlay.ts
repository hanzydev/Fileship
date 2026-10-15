import { join } from 'node:path';

import { BrowserWindow, nativeImage, screen } from 'electron';
import log from 'electron-log/main';

import type { OverlayFinish, OverlayPayload } from '../shared/ipc';
import {
    captureActiveMonitor,
    captureFullscreen,
    captureRect,
    physicalBounds,
    snapshotDisplays,
} from './capture';
import type { Rect } from './capture/native';
import { getSettings } from './store';
import { handleImage } from './tasks';
import { rendererUrl } from './windows';

const overlays = new Map<number, BrowserWindow>();
const pending = new Map<number, () => void>();
let isOpen = false;
let lastRegion: Rect | null = null;

const createOverlay = (display: Electron.Display) => {
    const window = new BrowserWindow({
        ...display.bounds,
        show: false,
        frame: false,
        resizable: false,
        movable: false,
        minimizable: false,
        maximizable: false,
        fullscreenable: false,
        skipTaskbar: true,
        alwaysOnTop: true,
        hasShadow: false,
        enableLargerThanScreen: true,
        backgroundColor: '#000000',
        webPreferences: {
            preload: join(__dirname, 'preload.cjs'),
            contextIsolation: true,
            sandbox: true,
            nodeIntegration: false,
            backgroundThrottling: false,
        },
    });

    window.setAlwaysOnTop(true, 'screen-saver');
    window.loadURL(rendererUrl(`/overlay?display=${display.id}`));
    window.on('closed', () => overlays.delete(display.id));

    overlays.set(display.id, window);
};

export const createOverlays = () => {
    for (const window of overlays.values()) window.destroy();
    overlays.clear();

    for (const display of screen.getAllDisplays()) createOverlay(display);
};

export const watchDisplays = () => {
    for (const event of ['display-added', 'display-removed', 'display-metrics-changed'] as const) {
        screen.on(event as 'display-added', () => {
            if (!isOpen) createOverlays();
        });
    }
};

export const markOverlayReady = (displayId: number) => pending.get(displayId)?.();

const reveal = (id: number, window: BrowserWindow, isActive: boolean) => {
    const done = () => {
        clearTimeout(timer);
        pending.delete(id);
        if (!window.isDestroyed()) window.setOpacity(1);
    };

    const timer = setTimeout(done, 400);
    pending.set(id, done);

    window.setOpacity(0);

    if (isActive) {
        window.show();
        window.focus();
    } else {
        window.showInactive();
    }
};

const hideOverlays = () => {
    isOpen = false;
    pending.clear();

    for (const window of overlays.values()) {
        if (!window.isDestroyed()) {
            window.webContents.send('overlay:close', null);
            window.hide();
        }
    }
};

export const openRegionCapture = () => {
    if (isOpen) return;
    isOpen = true;

    const cursor = screen.getCursorScreenPoint();
    const active = screen.getDisplayNearestPoint(cursor);
    const { capture } = getSettings();

    try {
        for (const shot of snapshotDisplays()) {
            const window = overlays.get(shot.display.id);
            if (!window || window.isDestroyed()) continue;

            const { bounds, scaleFactor } = shot.display;
            const toLocal = (rect: Rect) => {
                const dip = screen.screenToDipRect(null, rect);
                return { ...dip, x: dip.x - bounds.x, y: dip.y - bounds.y };
            };

            const payload: OverlayPayload = {
                displayId: shot.display.id,
                index: screen.getAllDisplays().findIndex((d) => d.id === shot.display.id),
                width: shot.physical.width,
                height: shot.physical.height,
                scaleFactor,
                pixels: shot.data,
                windows: shot.windows.map((item) => ({
                    ...toLocal(item),
                    title: item.title,
                    controls: item.controls.map(toLocal),
                })),
                cursor: { x: cursor.x - bounds.x, y: cursor.y - bounds.y },
                showMagnifier: capture.showMagnifier,
                quickCrop: capture.quickCrop,
                isActive: shot.display.id === active.id,
            };

            window.setBounds(bounds);
            window.webContents.send('overlay:open', payload);
        }

        for (const [id, window] of overlays) {
            if (!window.isDestroyed()) reveal(id, window, id === active.id);
        }
    } catch (error) {
        log.error('Could not open region capture', error);
        hideOverlays();
    }
};

export const finishRegionCapture = async (result: OverlayFinish) => {
    hideOverlays();

    if (result.type === 'cancel') return;

    await new Promise((resolve) => setTimeout(resolve, 50));

    if (result.type === 'fullscreen') return handleImage(captureFullscreen().image);
    if (result.type === 'monitor') {
        const display = screen.getAllDisplays()[result.index];
        if (!display) return handleImage(captureActiveMonitor().image);
        return handleImage(captureRect(physicalBounds(display)).image);
    }

    const display = screen.getAllDisplays().find((item) => item.id === result.displayId);

    if (display) {
        const { bounds } = display;

        lastRegion = screen.dipToScreenRect(null, {
            x: bounds.x + result.region.x,
            y: bounds.y + result.region.y,
            width: result.region.width,
            height: result.region.height,
        });
    }

    const pixels = Buffer.from(result.rgba.buffer, result.rgba.byteOffset, result.rgba.byteLength);

    for (let i = 0; i < pixels.length; i += 4) {
        const red = pixels[i]!;
        pixels[i] = pixels[i + 2]!;
        pixels[i + 2] = red;
    }

    return handleImage(
        nativeImage.createFromBitmap(pixels, { width: result.width, height: result.height }),
    );
};

export const captureLastRegion = () => {
    if (!lastRegion) return openRegionCapture();
    return handleImage(captureRect(lastRegion).image);
};
