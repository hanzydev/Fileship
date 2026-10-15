import { statSync } from 'node:fs';
import { basename, extname } from 'node:path';

import { dialog, Notification } from 'electron';
import log from 'electron-log/main';

import type { HotkeyAction } from '../shared/settings';
import { captureActiveMonitor, captureActiveWindow, captureFullscreen, type Shot } from './capture';
import { readFilePath, readImage, readText, writeText } from './clipboard';
import { captureLastRegion, openRegionCapture } from './overlay';
import { toggleRecording } from './recording/recorder';
import { getStore } from './store';
import { handleFile, handleImage } from './tasks';
import { shortenUrl } from './upload';
import { iconPath, showMainWindow } from './windows';

const MIME_TYPES: Record<string, string> = {
    '.png': 'image/png',
    '.jpg': 'image/jpeg',
    '.jpeg': 'image/jpeg',
    '.gif': 'image/gif',
    '.webp': 'image/webp',
    '.bmp': 'image/bmp',
    '.svg': 'image/svg+xml',
    '.mp4': 'video/mp4',
    '.webm': 'video/webm',
    '.mkv': 'video/x-matroska',
    '.mov': 'video/quicktime',
    '.mp3': 'audio/mpeg',
    '.wav': 'audio/wav',
    '.ogg': 'audio/ogg',
    '.pdf': 'application/pdf',
    '.zip': 'application/zip',
    '.txt': 'text/plain',
    '.md': 'text/markdown',
    '.json': 'application/json',
    '.html': 'text/html',
    '.css': 'text/css',
    '.js': 'text/javascript',
    '.ts': 'text/plain',
};

export const mimeTypeOf = (path: string) =>
    MIME_TYPES[extname(path).toLowerCase()] ?? 'application/octet-stream';

const notify = (title: string, body: string) => {
    if (Notification.isSupported()) new Notification({ title, body, icon: iconPath() }).show();
};

export const uploadPath = (path: string) =>
    handleFile({ name: basename(path), mimeType: mimeTypeOf(path), path });

const clipboardFile = async () => {
    const path = await readFilePath();

    try {
        return path && statSync(path).isFile() ? path : null;
    } catch {
        return null;
    }
};

const uploadClipboard = async () => {
    const path = await clipboardFile();
    if (path) return uploadPath(path);

    const image = await readImage();
    if (image) return handleImage(image, { kind: 'screenshot' });

    const text = await readText();
    if (!text.trim())
        return notify('Nothing to upload', 'Copy an image, a file or some text first.');

    return handleFile({
        name: 'clipboard.txt',
        mimeType: 'text/plain',
        data: Buffer.from(text, 'utf-8'),
    });
};

const uploadFromDialog = async () => {
    const result = await dialog.showOpenDialog({
        title: 'Upload to Fileship',
        properties: ['openFile', 'multiSelections'],
    });

    await Promise.all(result.filePaths.map(uploadPath));
};

const shortenClipboard = async () => {
    const text = (await readText()).trim();

    if (!/^https?:\/\/\S+$/i.test(text)) {
        return notify('No link to shorten', 'Copy an http or https link first.');
    }

    try {
        const shortUrl = await shortenUrl(text);
        await writeText(shortUrl);
        notify('Link shortened, copied', shortUrl);
    } catch (error: any) {
        notify('Could not shorten the link', error.message);
    }
};

const withShot = async (capture: Promise<Shot | null>) => {
    const shot = await capture;
    if (!shot) return notify('Nothing to capture', 'No window is active.');
    await handleImage(shot.image, { title: shot.title });
};

const handlers: Record<HotkeyAction, () => unknown> = {
    captureFullscreen: () => withShot(captureFullscreen()),
    captureWindow: () => withShot(captureActiveWindow()),
    captureMonitor: () => withShot(captureActiveMonitor()),
    captureRegion: () => openRegionCapture(),
    captureLastRegion: () => captureLastRegion(),
    recordVideo: () => toggleRecording('video'),
    recordGif: () => toggleRecording('gif'),
    uploadClipboard,
    uploadFile: uploadFromDialog,
    shortenClipboard,
    openMain: () => showMainWindow(),
};

const NEEDS_ACCOUNT: HotkeyAction[] = ['uploadClipboard', 'uploadFile', 'shortenClipboard'];

const REPEAT_WINDOW_MS = 400;
const lastRuns = new Map<HotkeyAction, number>();

export const runAction = async (action: HotkeyAction) => {
    const now = Date.now();
    const last = lastRuns.get(action);

    lastRuns.set(action, now);
    if (last && now - last < REPEAT_WINDOW_MS) return;

    if (NEEDS_ACCOUNT.includes(action) && !getStore().token) {
        showMainWindow('/connect');
        return;
    }

    try {
        await handlers[action]();
    } catch (error: any) {
        log.error(`Action ${action} failed`, error);
        notify('Something went wrong', error.message);
    }
};
