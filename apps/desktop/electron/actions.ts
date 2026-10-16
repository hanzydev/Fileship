import { statSync } from 'node:fs';
import { basename } from 'node:path';

import { dialog } from 'electron';
import log from 'electron-log/main';

import type { HotkeyAction } from '../shared/settings';
import { captureActiveMonitor, captureActiveWindow, captureFullscreen, type Shot } from './capture';
import { readFilePath, readImage, readText, writeText } from './clipboard';
import { t } from './i18n';
import { mimeTypeOf } from './mime';
import { captureText } from './ocr';
import { captureLastRegion, openRegionCapture } from './overlay';
import { toggleRecording } from './recording/recorder';
import { getStore } from './store';
import { handleFile, handleImage } from './tasks';
import { shortenUrl } from './upload';
import { notify, showMainWindow } from './windows';

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
    if (!text.trim()) return notify(t('app.nothingToUpload'), t('app.nothingToUploadDescription'));

    return handleFile({
        name: 'clipboard.txt',
        mimeType: 'text/plain',
        data: Buffer.from(text, 'utf-8'),
    });
};

const uploadFromDialog = async () => {
    const result = await dialog.showOpenDialog({
        title: t('app.uploadDialogTitle'),
        properties: ['openFile', 'multiSelections'],
    });

    await Promise.all(result.filePaths.map(uploadPath));
};

const shortenClipboard = async () => {
    const text = (await readText()).trim();

    if (!/^https?:\/\/\S+$/i.test(text)) {
        return notify(t('app.noLinkToShorten'), t('app.noLinkToShortenDescription'));
    }

    try {
        const shortUrl = await shortenUrl(text);
        await writeText(shortUrl);
        notify(t('app.linkShortened'), shortUrl);
    } catch (error: any) {
        notify(t('app.shortenFailed'), error.message);
    }
};

const withShot = async (capture: Promise<Shot | null>) => {
    const shot = await capture;
    if (!shot) return notify(t('app.nothingToCapture'), t('app.noActiveWindow'));
    await handleImage(shot.image, { title: shot.title });
};

const handlers: Record<HotkeyAction, () => unknown> = {
    captureFullscreen: () => withShot(captureFullscreen()),
    captureWindow: () => withShot(captureActiveWindow()),
    captureMonitor: () => withShot(captureActiveMonitor()),
    captureRegion: () => openRegionCapture(),
    captureLastRegion: () => captureLastRegion(),
    captureText,
    recordVideo: () => toggleRecording('video'),
    recordGif: () => toggleRecording('gif'),
    uploadClipboard,
    uploadFile: uploadFromDialog,
    shortenClipboard,
    openMain: () => showMainWindow(),
};

const NEEDS_ACCOUNT: HotkeyAction[] = [
    'captureText',
    'uploadClipboard',
    'uploadFile',
    'shortenClipboard',
];

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
        notify(t('app.somethingWentWrong'), error.message);
    }
};
