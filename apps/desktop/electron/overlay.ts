import { join } from 'node:path';

import { BrowserWindow, screen } from 'electron';
import log from 'electron-log/main';

import type { OverlayFinish, OverlayPayload, OverlayPurpose } from '../shared/ipc';
import {
    type Bitmap,
    bitmapFromRgba,
    captureRect,
    intersectRects,
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

let overlay: BrowserWindow | null = null;
let loading: Promise<boolean> | null = null;
let markLoaded: (() => void) | null = null;
let pendingReveal: { done: () => void; timer: NodeJS.Timeout } | null = null;
let idleTimer: NodeJS.Timeout | null = null;
let isOpen = false;
let purpose: OverlayPurpose = 'capture';
let onArea: ((rect: Rect) => void) | null = null;
let onImage: ((image: Bitmap) => void) | null = null;
let lastRegion: Rect | null = null;

const isSameRect = (a: Rect, b: Rect) =>
    a.x === b.x && a.y === b.y && a.width === b.width && a.height === b.height;

const createOverlay = () => {
    const window = new BrowserWindow({
        ...screen.screenToDipRect(null, virtualScreen()),
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

    loading = new Promise<boolean>((resolve) => {
        const timer = setTimeout(() => resolve(false), OVERLAY_LOAD_TIMEOUT_MS);

        markLoaded = () => {
            clearTimeout(timer);
            markLoaded = null;
            resolve(true);
        };
    });

    const handleGone = (reason: string) => {
        if (overlay !== window) return;

        log.warn('The capture overlay stopped', reason);
        resetOverlay();
    };

    window.webContents.on('render-process-gone', (_, details) => handleGone(details.reason));
    window.on('unresponsive', () => handleGone('unresponsive'));
    hardenWebContents(window);
    window.setAlwaysOnTop(true, 'screen-saver');
    window.loadURL(rendererUrl('/overlay'));
    window.on('closed', () => {
        if (overlay !== window) return;

        overlay = null;
        loading = null;
        markLoaded = null;
    });

    overlay = window;
};

const destroyOverlay = () => {
    if (idleTimer) clearTimeout(idleTimer);
    idleTimer = null;

    const window = overlay;
    overlay = null;
    loading = null;
    markLoaded = null;

    if (window && !window.isDestroyed()) window.destroy();
};

const clearPending = () => {
    if (pendingReveal) clearTimeout(pendingReveal.timer);
    pendingReveal = null;
};

const resetOverlay = () => {
    destroyOverlay();
    clearPending();
    isOpen = false;
    onArea = null;
};

const ensureOverlay = async () => {
    for (let attempt = 0; attempt < 2; attempt++) {
        if (!overlay || overlay.isDestroyed()) createOverlay();
        if (await loading) return overlay!;

        log.warn('The capture overlay did not load, recreating it');
        destroyOverlay();
    }

    throw new Error('The capture overlay did not load');
};

const placeOverlay = (window: BrowserWindow, bounds: Rect) => {
    window.setBounds(screen.screenToDipRect(null, bounds));
    if (isSameRect(screen.dipToScreenRect(window, window.getBounds()), bounds)) return;

    window.setBounds(screen.screenToDipRect(window, bounds));

    const placed = screen.dipToScreenRect(window, window.getBounds());
    if (!isSameRect(placed, bounds)) log.warn('The capture overlay is misplaced', placed, bounds);
};

export const markOverlayLoaded = () => markLoaded?.();

export const watchDisplays = () => {
    for (const event of ['display-added', 'display-removed', 'display-metrics-changed'] as const) {
        screen.on(event as 'display-added', () => {
            if (!isOpen) destroyOverlay();
        });
    }
};

export const markOverlayReady = () => pendingReveal?.done();

const reveal = (window: BrowserWindow) => {
    const done = () => {
        clearTimeout(timer);
        pendingReveal = null;
        if (!window.isDestroyed()) window.setOpacity(1);
    };

    const timer = setTimeout(done, 400);
    pendingReveal = { done, timer };

    window.setOpacity(0);
    window.show();
    window.focus();

    try {
        native.forceForeground(windowHandle(window));
    } catch (error) {
        log.warn('Could not focus the capture overlay', error);
    }
};

const hideOverlay = () => {
    isOpen = false;
    clearPending();

    if (idleTimer) clearTimeout(idleTimer);
    idleTimer = setTimeout(destroyOverlay, OVERLAY_IDLE_MS);

    if (overlay && !overlay.isDestroyed()) {
        overlay.webContents.send('overlay:close', null);
        overlay.hide();
    }
};

export const openRegionCapture = async (
    nextPurpose: OverlayPurpose = 'capture',
    nextOnArea: ((rect: Rect) => void) | null = null,
    nextOnImage: ((image: Bitmap) => void) | null = null,
) => {
    if (isOpen) return;
    isOpen = true;
    purpose = nextPurpose;
    onArea = nextOnArea;
    onImage = nextOnImage;

    if (idleTimer) clearTimeout(idleTimer);
    idleTimer = null;

    const { capture } = getSettings();

    try {
        const [shots, window] = await Promise.all([snapshotDisplays(), ensureOverlay()]);
        const bounds = virtualScreen();
        const primary = screen.getPrimaryDisplay();

        const payload: OverlayPayload = {
            bounds,
            displays: shots.map((shot) => ({
                id: shot.display.id,
                rect: shot.physical,
                scaleFactor: shot.display.scaleFactor,
                isPrimary: shot.display.id === primary.id,
                pixels: shot.data,
            })),
            windows: shots[0]?.windows ?? [],
            cursor: screen.dipToScreenPoint(screen.getCursorScreenPoint()),
            showMagnifier: capture.showMagnifier,
            purpose,
        };

        placeOverlay(window, bounds);
        window.webContents.send('overlay:open', payload);
        reveal(window);
    } catch (error) {
        log.error('Could not open region capture', error);
        hideOverlay();
    }
};

const finishArea = (result: OverlayFinish) => {
    const callback = onArea;
    onArea = null;
    if (!callback || result.type === 'cancel') return;

    const rect = intersectRects(result.rect, virtualScreen());
    if (rect && rect.width >= 2 && rect.height >= 2) callback(rect);
};

const finishImage = (result: OverlayFinish) => {
    const callback = onImage;
    onImage = null;
    if (callback && result.type === 'region')
        callback(bitmapFromRgba(result.rgba, result.width, result.height));
};

export const finishRegionCapture = async (result: OverlayFinish) => {
    hideOverlay();

    if (purpose === 'ocr') return finishImage(result);
    if (purpose !== 'capture') return finishArea(result);
    if (result.type !== 'region') return;

    lastRegion = intersectRects(result.rect, virtualScreen()) ?? lastRegion;
    playShutter();

    return handleImage(bitmapFromRgba(result.rgba, result.width, result.height), {
        isEdited: true,
    });
};

export const captureLastRegion = async () => {
    if (!lastRegion) return openRegionCapture();
    return handleImage((await captureRect(lastRegion)).image);
};
