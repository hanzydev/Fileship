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
    playShutter,
    snapshotDisplays,
    virtualScreen,
    windowHandle,
} from './capture';
import { native, type Rect } from './native';
import { getSettings } from './store';
import { handleImage } from './tasks';
import { hardenWebContents, rendererUrl } from './windows';

const OVERLAY_IDLE_MS = 2 * 60 * 1000;
const OVERLAY_LOAD_TIMEOUT_MS = 5000;

const overlays = new Map<number, BrowserWindow>();
const loading = new Map<number, Promise<boolean>>();
const loaded = new Map<number, () => void>();
const pending = new Map<number, { done: () => void; timer: NodeJS.Timeout }>();
let idleTimer: NodeJS.Timeout | null = null;
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

    const isLoaded = new Promise<boolean>((resolve) => {
        const timer = setTimeout(() => resolve(false), OVERLAY_LOAD_TIMEOUT_MS);

        loaded.set(display.id, () => {
            clearTimeout(timer);
            loaded.delete(display.id);
            resolve(true);
        });
    });

    const handleGone = (reason: string) => {
        if (overlays.get(display.id) !== window) return;

        log.warn('A capture overlay stopped', reason);
        resetOverlays();
    };

    window.webContents.on('render-process-gone', (_, details) => handleGone(details.reason));
    window.on('unresponsive', () => handleGone('unresponsive'));
    hardenWebContents(window);
    window.setAlwaysOnTop(true, 'screen-saver');
    window.loadURL(rendererUrl(`/overlay?display=${display.id}`));
    window.on('closed', () => {
        if (overlays.get(display.id) !== window) return;

        overlays.delete(display.id);
        loading.delete(display.id);
        loaded.delete(display.id);
    });

    overlays.set(display.id, window);
    loading.set(display.id, isLoaded);
};

const destroyOverlay = (id: number) => {
    const window = overlays.get(id);

    overlays.delete(id);
    loading.delete(id);
    loaded.delete(id);

    if (window && !window.isDestroyed()) window.destroy();
};

const destroyOverlays = () => {
    if (idleTimer) clearTimeout(idleTimer);
    idleTimer = null;

    for (const id of [...overlays.keys()]) destroyOverlay(id);
};

const clearPending = () => {
    for (const { timer } of pending.values()) clearTimeout(timer);
    pending.clear();
};

const resetOverlays = () => {
    destroyOverlays();
    clearPending();
    isOpen = false;
    onArea = null;
};

const createMissing = (displays: Electron.Display[]) => {
    for (const display of displays) {
        if (!overlays.get(display.id) || overlays.get(display.id)!.isDestroyed()) {
            createOverlay(display);
        }
    }

    return Promise.all(displays.map((display) => loading.get(display.id) ?? false));
};

const ensureOverlays = async () => {
    const displays = screen.getAllDisplays();
    const results = await createMissing(displays);
    const failed = displays.filter((_, index) => !results[index]);

    if (!failed.length) return;

    log.warn('Capture overlays did not load, recreating them', failed.length);
    for (const display of failed) destroyOverlay(display.id);

    if ((await createMissing(failed)).some((isLoaded) => !isLoaded)) {
        throw new Error('The capture overlay did not load');
    }
};

export const markOverlayLoaded = (displayId: number) => loaded.get(displayId)?.();

export const watchDisplays = () => {
    for (const event of ['display-added', 'display-removed', 'display-metrics-changed'] as const) {
        screen.on(event as 'display-added', () => {
            if (!isOpen) destroyOverlays();
        });
    }
};

export const markOverlayReady = (displayId: number) => pending.get(displayId)?.done();

const reveal = (id: number, window: BrowserWindow, isActive: boolean) => {
    const done = () => {
        clearTimeout(timer);
        pending.delete(id);
        if (!window.isDestroyed()) window.setOpacity(1);
    };

    const timer = setTimeout(done, 400);
    pending.set(id, { done, timer });

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
    clearPending();

    if (idleTimer) clearTimeout(idleTimer);
    idleTimer = setTimeout(destroyOverlays, OVERLAY_IDLE_MS);

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

    if (idleTimer) clearTimeout(idleTimer);
    idleTimer = null;

    const cursor = screen.getCursorScreenPoint();
    const active = screen.getDisplayNearestPoint(cursor);
    const { capture } = getSettings();

    try {
        const [shots] = await Promise.all([snapshotDisplays(), ensureOverlays()]);

        for (const shot of shots) {
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
    playShutter();

    return handleImage(bitmapFromRgba(result.rgba, result.width, result.height), {
        isEdited: true,
    });
};

export const captureLastRegion = async () => {
    if (!lastRegion) return openRegionCapture();
    return handleImage((await captureRect(lastRegion)).image);
};
