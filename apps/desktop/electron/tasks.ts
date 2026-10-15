import { mkdirSync, writeFileSync } from 'node:fs';
import { join } from 'node:path';

import { app, Notification, shell } from 'electron';
import log from 'electron-log/main';

import type { HistoryKind } from '../shared/ipc';
import { writeImage, writeText } from './clipboard';
import { addHistory, updateHistory } from './history';
import { getSettings } from './store';
import { shortenUrl, uploadFile } from './upload';
import { iconPath } from './windows';

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

const notify = (title: string, body: string, url?: string) => {
    if (!Notification.isSupported()) return;

    const notification = new Notification({ title, body, icon: iconPath() });
    if (url) notification.on('click', () => shell.openExternal(url));
    notification.show();
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
    if (afterUpload.openUrl) shell.openExternal(url);

    if (afterUpload.notify) {
        notify(afterUpload.copyUrl ? 'Uploaded, link copied' : 'Uploaded', url, url);
    }

    return url;
};

export const uploadAndFinish = async (
    historyId: string,
    file: { name: string; mimeType: string; data: Buffer },
) => {
    try {
        const uploaded = await uploadFile({ ...file, id: historyId });
        return await finishUpload(historyId, uploaded);
    } catch (error: any) {
        log.warn('Upload failed', error);
        updateHistory(historyId, { error: error.message });
        notify('Upload failed', error.message);
        return null;
    }
};

export const handleImage = async (
    image: Electron.NativeImage,
    options: { title?: string; kind?: HistoryKind } = {},
) => {
    const { afterCapture, save } = getSettings();
    const png = image.toPNG();
    const name = `${formatFileName(save.pattern, options.title) || 'capture'}.png`;

    let localPath: string | null = null;

    if (afterCapture.copyImage) await writeImage(image);

    if (afterCapture.saveToFile) {
        const folder = saveFolder();
        mkdirSync(folder, { recursive: true });
        localPath = join(folder, name);
        writeFileSync(localPath, png);
    }

    const entry = addHistory({
        kind: options.kind ?? 'screenshot',
        name,
        size: png.length,
        localPath,
        thumbnail: `data:image/jpeg;base64,${image.resize({ width: 480 }).toJPEG(75).toString('base64')}`,
    });

    if (afterCapture.upload) {
        await uploadAndFinish(entry.id, { name, mimeType: 'image/png', data: png });
    } else if (!afterCapture.copyImage && !afterCapture.saveToFile) {
        await writeImage(image);
        notify('Screenshot copied', 'Uploading is turned off in the after capture settings.');
    }

    return entry;
};

export const handleFile = async (file: { name: string; mimeType: string; data: Buffer }) => {
    const entry = addHistory({ kind: 'file', name: file.name, size: file.data.length });
    await uploadAndFinish(entry.id, file);
    return entry;
};
