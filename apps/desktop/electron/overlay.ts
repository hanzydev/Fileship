import { join } from 'node:path';

import { BrowserWindow, screen } from 'electron';
import log from 'electron-log/main';

import type { OverlayFinish, OverlayPayload, OverlayPurpose } from '../shared/ipc';
import {
    bitmapFromRgba,
    captureActiveMonitor,
    captureFullscreen,
    captureRect,
    physicalBounds,
    snapshotDisplays,
    virtualScreen,
    windowHandle,
} from './capture';
import { native, type Rect } from './native';
import { getSettings } from './store';
import { handleImage } from './tasks';
import { rendererUrl } from './windows';

const overlays = new Map<number, BrowserWindow>();
const pending = new Map<number, () => void>();
let isOpen = false;
let purpose: OverlayPurpose = 'capture';
let onArea: ((rect: Rect) => void) | null = null;
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

        try {
            native.forceForeground(windowHandle(window));
        } catch (error) {
            log.warn('Could not focus the capture overlay', error);
        }
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

export const openRegionCapture = async (
    nextPurpose: OverlayPurpose = 'capture',
    nextOnArea: ((rect: Rect) => void) | null = null,
) => {
    if (isOpen) return;
    isOpen = true;
    purpose = nextPurpose;
    onArea = nextOnArea;

    const cursor = screen.getCursorScreenPoint();
    const active = screen.getDisplayNearestPoint(cursor);
    const { capture } = getSettings();

    try {
        for (const shot of await snapshotDisplays()) {
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
                purpose,
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

const toScreenRect = (displayId: number, region: Rect) => {
    const display = screen.getAllDisplays().find((item) => item.id === displayId);
    if (!display) return null;

    return screen.dipToScreenRect(null, {
        x: display.bounds.x + region.x,
        y: display.bounds.y + region.y,
        width: region.width,
        height: region.height,
    });
};

const finishArea = (result: OverlayFinish) => {
    const callback = onArea;
    onArea = null;
    if (!callback || result.type === 'cancel') return;

    if (result.type === 'fullscreen') return callback(virtualScreen());

    if (result.type === 'monitor') {
        const display = screen.getAllDisplays()[result.index];
        if (display) callback(physicalBounds(display));
        return;
    }

    if (result.type === 'area' || result.type === 'region') {
        const rect = toScreenRect(result.displayId, result.region);
        if (rect && rect.width >= 2 && rect.height >= 2) callback(rect);
    }
};

export const finishRegionCapture = async (result: OverlayFinish) => {
    hideOverlays();

    if (purpose !== 'capture') return finishArea(result);
    if (result.type === 'cancel' || result.type === 'area') return;

    await new Promise((resolve) => setTimeout(resolve, 50));

    if (result.type === 'fullscreen') return handleImage((await captureFullscreen()).image);
    if (result.type === 'monitor') {
        const display = screen.getAllDisplays()[result.index];
        if (!display) return handleImage((await captureActiveMonitor()).image);
        return handleImage((await captureRect(physicalBounds(display))).image);
    }

    lastRegion = toScreenRect(result.displayId, result.region) ?? lastRegion;

    return handleImage(bitmapFromRgba(result.rgba, result.width, result.height), {
        isEdited: true,
    });
};

export const captureLastRegion = async () => {
    if (!lastRegion) return openRegionCapture();
    return handleImage((await captureRect(lastRegion)).image);
};
