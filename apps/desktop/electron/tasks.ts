import { existsSync, mkdirSync, statSync, writeFileSync } from 'node:fs';
import { copyFile, mkdir, rename, rm, writeFile } from 'node:fs/promises';
import { extname, join } from 'node:path';

import { app, nativeImage, shell } from 'electron';
import log from 'electron-log/main';

import type { HistoryKind } from '../shared/ipc';
import { type Bitmap, bitmapFromImage, encodePng, encodeThumbnail, playUploaded } from './capture';
import { writeImage, writeText } from './clipboard';
import { addHistory, updateHistory } from './history';
import { t } from './i18n';
import { getSettings } from './store';
import { notifyIdle } from './updater';
import { shortenUrl, uploadFile, type UploadSource } from './upload';
import { notify, openWebUrl } from './windows';

let activeTasks = 0;

export const isWorking = () => activeTasks > 0;

export const track = async <T>(task: () => Promise<T>) => {
    activeTasks++;

    try {
        return await task();
    } finally {
        activeTasks--;
        notifyIdle();
    }
};

const pad = (value: number) => String(value).padStart(2, '0');

export const formatFileName = (pattern: string, title = '') => {
    const now = new Date();
    const random = () => Math.random().toString(36).slice(2, 12);

    return pattern
        .replaceAll('%yyyy', String(now.getFullYear()))
        .replaceAll('%mo', pad(now.getMonth() + 1))
        .replaceAll('%d', pad(now.getDate()))
        .replaceAll('%h', pad(now.getHours()))
        .replaceAll('%mi', pad(now.getMinutes()))
        .replaceAll('%ms', String(now.getMilliseconds()).padStart(3, '0'))
        .replaceAll('%s', pad(now.getSeconds()))
        .replaceAll('%pn', title)
        .replaceAll('%ra', random())
        .replace(/[\\/:*?"<>|]/g, '')
        .trim();
};

export const saveFolder = () =>
    getSettings().save.folder || join(app.getPath('pictures'), 'Fileship');

export const uniquePath = (folder: string, name: string) => {
    const extension = extname(name);
    const base = name.slice(0, name.length - extension.length);
    let path = join(folder, name);

    for (let index = 2; existsSync(path); index++) {
        path = join(folder, `${base} (${index})${extension}`);
    }

    return path;
};

export const finishUpload = async (historyId: string, file: { url: string; id: string }) => {
    const { afterUpload } = getSettings();

    let url = file.url;
    let shortUrl: string | null = null;

    if (afterUpload.shortenUrl) {
        shortUrl = await shortenUrl(url).catch((error) => {
            log.warn('Could not shorten the link', error);
            return null;
        });

        if (shortUrl) url = shortUrl;
    }

    updateHistory(historyId, { url: file.url, shortUrl, fileId: file.id, error: null });

    if (afterUpload.copyUrl) await writeText(url);
    playUploaded();
    if (afterUpload.openUrl) openWebUrl(url);

    if (afterUpload.notify) {
        notify(t(afterUpload.copyUrl ? 'tasks.uploadedLinkCopied' : 'tasks.uploaded'), url, () =>
            openWebUrl(url),
        );
    }

    return url;
};

export const uploadAndFinish = async (
    historyId: string,
    file: UploadSource,
    onFailure: (message: string) => unknown = (message) => notify(t('tasks.uploadFailed'), message),
) => {
    try {
        const uploaded = await uploadFile({ ...file, id: historyId });
        return await finishUpload(historyId, uploaded);
    } catch (error: any) {
        log.warn('Upload failed', error);
        updateHistory(historyId, { error: error.message });
        await onFailure(error.message);
        return null;
    }
};

const notifySaved = (path: string) =>
    notify(t('tasks.imageSaved'), path, () => shell.showItemInFolder(path));

const keepImage = async (historyId: string, png: Buffer, name: string, message: string) => {
    try {
        const folder = saveFolder();
        await mkdir(folder, { recursive: true });

        const kept = uniquePath(folder, name);
        await writeFile(kept, png);

        updateHistory(historyId, { localPath: kept });
        notify(t('tasks.imageKept'), kept, () => shell.showItemInFolder(kept));
    } catch (error) {
        log.warn('Could not keep the screenshot', error);
        notify(t('tasks.uploadFailed'), message);
    }
};

const processImage = async (
    image: Bitmap,
    options: { title?: string; kind?: HistoryKind } = {},
) => {
    const { afterCapture, save } = getSettings();
    const name = `${formatFileName(save.pattern, options.title) || 'capture'}.png`;

    const [png, thumbnail] = await Promise.all([encodePng(image), encodeThumbnail(image)]);

    let localPath: string | null = null;

    if (afterCapture.copyImage) await writeImage(png);

    if (afterCapture.saveToFile) {
        const folder = saveFolder();
        mkdirSync(folder, { recursive: true });
        localPath = uniquePath(folder, name);
        writeFileSync(localPath, png);
    }

    const entry = addHistory({
        kind: options.kind ?? 'screenshot',
        name,
        size: png.length,
        localPath,
        thumbnail,
    });

    if (afterCapture.upload) {
        await uploadAndFinish(
            entry.id,
            { name, mimeType: 'image/png', data: png },
            localPath ? undefined : (message) => keepImage(entry.id, png, name, message),
        );
    } else if (localPath) {
        notifySaved(localPath);
    } else {
        if (!afterCapture.copyImage) await writeImage(png);
        notify(t('tasks.screenshotCopied'), t('tasks.uploadsOff'));
    }

    return entry;
};

const fileThumbnail = async (file: UploadSource) => {
    if (!file.mimeType.startsWith('image/')) return null;

    const image = bitmapFromImage(
        file.data
            ? nativeImage.createFromBuffer(file.data)
            : nativeImage.createFromPath(file.path!),
    );

    return image ? encodeThumbnail(image) : null;
};

const processFile = async (file: UploadSource) => {
    const size = file.data ? file.data.length : statSync(file.path!).size;
    const entry = addHistory({
        kind: 'file',
        name: file.name,
        size,
        localPath: file.path ?? null,
        thumbnail: await fileThumbnail(file).catch(() => null),
    });
    await uploadAndFinish(entry.id, file);
    return entry;
};

const moveFile = async (from: string, to: string) => {
    try {
        await rename(from, to);
    } catch (error: any) {
        if (error.code !== 'EXDEV') throw error;

        await copyFile(from, to);
        await rm(from, { force: true });
    }
};

const keepRecording = async (historyId: string, path: string, name: string, message: string) => {
    try {
        const folder = saveFolder();
        await mkdir(folder, { recursive: true });

        const kept = uniquePath(folder, name);
        await moveFile(path, kept);

        updateHistory(historyId, { localPath: kept });
        notify(t('tasks.recordingKept'), kept, () => shell.showItemInFolder(kept));
    } catch (error) {
        log.warn('Could not keep the recording', error);
        notify(t('tasks.uploadFailed'), message);
    }
};

const processRecording = async (file: {
    kind: HistoryKind;
    path: string;
    name: string;
    mimeType: string;
    thumbnail: string | null;
    isSaved: boolean;
}) => {
    const entry = addHistory({
        kind: file.kind,
        name: file.name,
        size: statSync(file.path).size,
        localPath: file.isSaved ? file.path : null,
        thumbnail: file.thumbnail,
    });

    if (getSettings().afterCapture.upload) {
        await uploadAndFinish(
            entry.id,
            { name: file.name, mimeType: file.mimeType, path: file.path },
            file.isSaved
                ? undefined
                : (message) => keepRecording(entry.id, file.path, file.name, message),
        );
    } else {
        notify(t('tasks.recordingSaved'), file.path, () => shell.showItemInFolder(file.path));
    }

    return entry;
};

export const handleImage = (...args: Parameters<typeof processImage>) =>
    track(() => processImage(...args));

export const handleFile = (...args: Parameters<typeof processFile>) =>
    track(() => processFile(...args));

export const handleRecording = (...args: Parameters<typeof processRecording>) =>
    track(() => processRecording(...args));
