import { statSync } from 'node:fs';
import { basename } from 'node:path';

import { dialog } from 'electron';
import log from 'electron-log/main';

import type { HotkeyAction } from '../shared/settings';
import {
    captureActiveMonitor,
    captureActiveWindow,
    captureFullscreen,
    encodePng,
    playUploaded,
    type Shot,
} from './capture';
import { readFilePath, readImage, readText, writeText } from './clipboard';
import { addHistory, updateHistory } from './history';
import { t } from './i18n';
import { mimeTypeOf } from './mime';
import { captureText } from './ocr';
import { captureLastRegion, openRegionCapture } from './overlay';
import { toggleRecording } from './recording/recorder';
import { getSettings, getStore } from './store';
import { formatFileName, handleFile, handleImage } from './tasks';
import { shortenUrl } from './upload';
import { notify, openWebUrl, showMainWindow } from './windows';

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
    if (image) {
        const name = `${formatFileName(getSettings().save.pattern) || 'clipboard'}.png`;
        return handleFile({ name, mimeType: 'image/png', data: await encodePng(image) });
    }

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

    const entry = addHistory({ kind: 'link', name: text, size: 0 });

    try {
        const shortUrl = await shortenUrl(text);
        const { afterUpload } = getSettings();

        updateHistory(entry.id, { url: shortUrl });

        if (afterUpload.copyUrl) await writeText(shortUrl);
        playUploaded();
        if (afterUpload.openUrl) openWebUrl(shortUrl);

        if (afterUpload.notify) {
            notify(
                t(afterUpload.copyUrl ? 'app.linkShortened' : 'app.linkShortenedOnly'),
                shortUrl,
                () => openWebUrl(shortUrl),
            );
        }
    } catch (error: any) {
        updateHistory(entry.id, { error: error.message });
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

export const isConnected = () => !!getStore().token;

const runGuarded = async (name: string, needsAccount: boolean, task: () => unknown) => {
    if (needsAccount && !isConnected()) {
        showMainWindow('/connect');
        return;
    }

    try {
        await task();
    } catch (error: any) {
        log.error(`${name} failed`, error);
        notify(t('app.somethingWentWrong'), error.message);
    }
};

export const runAction = async (action: HotkeyAction) => {
    const now = Date.now();
    const last = lastRuns.get(action);

    lastRuns.set(action, now);
    if (last && now - last < REPEAT_WINDOW_MS) return;

    await runGuarded(`Action ${action}`, NEEDS_ACCOUNT.includes(action), handlers[action]);
};

export const uploadPaths = (paths: string[]) =>
    runGuarded('Upload', true, () => Promise.all(paths.map(uploadPath)));
