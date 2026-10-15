import { mkdirSync, statSync, writeFileSync } from 'node:fs';
import { writeFile } from 'node:fs/promises';
import { join } from 'node:path';

import { app, dialog, nativeImage, shell } from 'electron';
import log from 'electron-log/main';

import type { HistoryKind } from '../shared/ipc';
import { type Bitmap, bitmapFromImage, encodePng, encodeThumbnail } from './capture';
import { writeImage, writeText } from './clipboard';
import { openEditor } from './editor';
import { addHistory, getHistory, updateHistory } from './history';
import { getSettings } from './store';
import { shortenUrl, uploadFile, type UploadSource } from './upload';
import { notify, openWebUrl } from './windows';

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
    if (afterUpload.openUrl) openWebUrl(url);

    if (afterUpload.notify) {
        notify(afterUpload.copyUrl ? 'Uploaded, link copied' : 'Uploaded', url, () =>
            openWebUrl(url),
        );
    }

    return url;
};

export const uploadAndFinish = async (historyId: string, file: UploadSource) => {
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

const continueLabel = () => {
    const { afterCapture } = getSettings();
    if (afterCapture.upload) return 'Upload';
    if (afterCapture.saveToFile) return 'Save';
    return 'Copy';
};

const saveImageAs = async (png: Buffer, name: string) => {
    const result = await dialog.showSaveDialog({
        defaultPath: join(saveFolder(), name),
        filters: [{ name: 'PNG image', extensions: ['png'] }],
    });

    if (result.canceled || !result.filePath) return;

    await writeFile(result.filePath, png);
    notify('Image saved', result.filePath);
};

const runEditor = async (image: Bitmap, name: string) => {
    const result = await openEditor(
        { png: await encodePng(image), width: image.width, height: image.height },
        { name, continueLabel: continueLabel() },
    );

    if (result.action === 'copy') {
        await writeImage(await encodePng(result.image));
        notify('Image copied', 'Paste it anywhere.');
    } else if (result.action === 'save') {
        await saveImageAs(await encodePng(result.image), name);
    }

    return result.action === 'continue' ? result.image : null;
};

export const handleImage = async (
    captured: Bitmap,
    options: { title?: string; kind?: HistoryKind; isEdited?: boolean } = {},
) => {
    const { afterCapture, save } = getSettings();
    const name = `${formatFileName(save.pattern, options.title) || 'capture'}.png`;

    const image =
        afterCapture.annotate && !options.isEdited ? await runEditor(captured, name) : captured;

    if (!image) return null;

    const [png, thumbnail] = await Promise.all([encodePng(image), encodeThumbnail(image)]);

    let localPath: string | null = null;

    if (afterCapture.copyImage) await writeImage(png);

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
        thumbnail,
    });

    if (afterCapture.upload) {
        await uploadAndFinish(entry.id, { name, mimeType: 'image/png', data: png });
    } else if (!afterCapture.copyImage && !afterCapture.saveToFile) {
        await writeImage(png);
        notify('Screenshot copied', 'Uploading is turned off in the after capture settings.');
    }

    return entry;
};

export const handleFile = async (file: UploadSource) => {
    const size = file.data ? file.data.length : statSync(file.path!).size;
    const entry = addHistory({ kind: 'file', name: file.name, size });
    await uploadAndFinish(entry.id, file);
    return entry;
};

export const handleRecording = async (file: {
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
        await uploadAndFinish(entry.id, {
            name: file.name,
            mimeType: file.mimeType,
            path: file.path,
        });
    } else {
        notify('Recording saved', file.path, () => shell.showItemInFolder(file.path));
    }

    return entry;
};

export const editHistoryImage = async (id: string) => {
    const item = getHistory(id);
    if (!item?.localPath) throw new Error('Only screenshots saved on this computer can be edited.');

    const image = bitmapFromImage(nativeImage.createFromPath(item.localPath));
    if (!image) throw new Error('The saved screenshot could not be opened.');

    const edited = await runEditor(image, item.name);
    if (edited) await handleImage(edited, { isEdited: true });
};
