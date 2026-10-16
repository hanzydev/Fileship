import { randomBytes } from 'node:crypto';
import { open, stat } from 'node:fs/promises';

import type { UploadProgress } from '../shared/ipc';
import type { Settings } from '../shared/settings';
import { handleUnauthorized } from './account';
import { api, ApiError } from './api';
import { t } from './i18n';
import { getSettings, getStore } from './store';
import { refreshTray } from './tray';
import { notifyIdle } from './updater';
import { broadcast, getMainWindow } from './windows';

const MAX_ATTEMPTS = 3;
const MAX_CONCURRENT = 3;
const LIMITS_TTL_MS = 5 * 60 * 1000;

export type UploadedFile = {
    id: string;
    name: string;
    fileName: string;
    url: string;
    directUrl: string;
};

export type UploadSource = {
    name: string;
    mimeType: string;
    options?: Partial<Settings['upload']>;
    onProgress?: (sent: number, total: number) => void;
} & ({ data: Buffer; path?: never } | { path: string; data?: never });

type Job = UploadSource & {
    id: string;
    resolve: (file: UploadedFile) => void;
    reject: (error: Error) => void;
};

const queue: Job[] = [];
const progress = new Map<string, UploadProgress>();
let running = 0;
let limits: { chunkSize: number; concurrency: number; at: number } | null = null;

export const getUploadSummary = () => {
    const items = [...progress.values()];
    if (!items.length) return null;

    return {
        count: items.length,
        progress: Math.round(items.reduce((sum, item) => sum + item.progress, 0) / items.length),
    };
};

const publish = () => {
    const summary = getUploadSummary();

    broadcast('uploads:progress', [...progress.values()]);
    getMainWindow()?.setProgressBar(summary ? Math.max(summary.progress, 1) / 100 : -1);
    refreshTray();
};

export const isUploading = () => running > 0 || queue.length > 0;

const getUploadLimits = async () => {
    if (limits && Date.now() - limits.at < LIMITS_TTL_MS) return limits;

    const settings = await api<{ fileChunkSize?: number; uploadConcurrency?: number }>(
        '/api/settings/public',
    ).catch(() => null);

    const next = {
        chunkSize: (settings?.fileChunkSize ?? 25) * 1024 * 1024,
        concurrency: Math.max(1, settings?.uploadConcurrency ?? 1),
        at: Date.now(),
    };

    if (settings) limits = next;
    return next;
};

export const resetUploadSettings = () => {
    limits = null;
};

const sendChunk = async (form: () => FormData) => {
    for (let attempt = 1; ; attempt++) {
        try {
            return await api<UploadedFile | undefined>('/api/files', {
                method: 'POST',
                body: form(),
                timeout: 120_000,
            });
        } catch (error) {
            if (error instanceof ApiError && error.status === 401) handleUnauthorized();

            const isRetryable =
                error instanceof ApiError && (error.status === 0 || error.status >= 500);

            if (!isRetryable || attempt >= MAX_ATTEMPTS) throw error;

            await new Promise((resolve) => setTimeout(resolve, attempt * 1000));
        }
    }
};

const runJob = async (job: Job) => {
    const upload = { ...getSettings().upload, ...job.options };
    const { chunkSize: size, concurrency } = await getUploadLimits();
    const uploadId = randomBytes(12).toString('base64url');
    const fileSize = job.data ? job.data.length : (await stat(job.path!)).size;
    const totalChunks = Math.max(1, Math.ceil(fileSize / size));
    const handle = job.path ? await open(job.path, 'r') : null;

    const readChunk = async (offset: number) => {
        const length = Math.min(size, fileSize - offset);
        if (!handle) return job.data!.subarray(offset, offset + length);

        const buffer = Buffer.alloc(length);
        await handle.read(buffer, 0, length, offset);
        return buffer;
    };

    let result: UploadedFile | undefined;
    let next = 0;
    let sent = 0;
    let failure: unknown = null;

    const sendPart = async (index: number) => {
        const offset = index * size;
        const data = await readChunk(offset);
        const chunk = new Blob([new Uint8Array(data)], { type: job.mimeType });

        const response = await sendChunk(() => {
            const form = new FormData();

            form.append('uploadId', uploadId);
            form.append('currentChunk', String(index + 1));
            form.append('totalChunks', String(totalChunks));
            form.append('chunkOffset', String(offset));
            form.append('fileNameType', upload.fileNameType);
            if (upload.expiration) form.append('expiration', String(upload.expiration));
            if (upload.maxViews) form.append('maxViews', String(upload.maxViews));
            if (upload.compression) form.append('compression', String(upload.compression));
            if (upload.password) form.append('password', upload.password);
            if (upload.folderId) form.append('folderId', upload.folderId);
            form.append('file', chunk, job.name);

            return form;
        });

        if (response) result = response;
        sent += data.length;

        progress.set(job.id, {
            ...progress.get(job.id)!,
            progress: fileSize ? Math.round((sent / fileSize) * 100) : 100,
        });

        publish();
        job.onProgress?.(sent, fileSize);
    };

    const worker = async () => {
        while (!failure && next < totalChunks) {
            try {
                await sendPart(next++);
            } catch (error) {
                failure ??= error;
            }
        }
    };

    try {
        await Promise.all(Array.from({ length: Math.min(concurrency, totalChunks) }, worker));
    } finally {
        await handle?.close();
    }

    if (failure) throw failure;

    if (!result?.url) throw new Error(t('tasks.noLink'));
    return result;
};

const pump = () => {
    while (running < MAX_CONCURRENT && queue.length) {
        const job = queue.shift()!;
        running++;

        runJob(job)
            .then(job.resolve, job.reject)
            .finally(() => {
                running--;
                progress.delete(job.id);
                publish();
                pump();
                if (!isUploading()) notifyIdle();
            });
    }
};

export const uploadFile = (file: UploadSource & { id?: string }) => {
    if (!getStore().token) return Promise.reject(new Error(t('tasks.logInToUpload')));

    const id = file.id ?? randomBytes(8).toString('hex');

    return new Promise<UploadedFile>((resolve, reject) => {
        queue.push({ ...file, id, resolve, reject } as Job);
        progress.set(id, { id, name: file.name, progress: 0 });
        publish();
        pump();
    });
};

export const getUploadProgress = () => [...progress.values()];

export const shortenUrl = async (url: string) => {
    try {
        const link = await api<{ shortUrl: string }>('/api/links', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ url }),
        });

        return link.shortUrl;
    } catch (error) {
        if (error instanceof ApiError && error.status === 401) handleUnauthorized();
        throw error;
    }
};
