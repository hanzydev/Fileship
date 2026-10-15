import { randomBytes } from 'node:crypto';
import { open, stat } from 'node:fs/promises';

import type { UploadProgress } from '../shared/ipc';
import type { Settings } from '../shared/settings';
import { handleUnauthorized } from './account';
import { api, ApiError } from './api';
import { getSettings, getStore } from './store';
import { notifyIdle } from './updater';
import { broadcast } from './windows';

const MAX_ATTEMPTS = 3;
const MAX_CONCURRENT = 3;

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
} & ({ data: Buffer; path?: never } | { path: string; data?: never });

type Job = UploadSource & {
    id: string;
    resolve: (file: UploadedFile) => void;
    reject: (error: Error) => void;
};

const queue: Job[] = [];
const progress = new Map<string, UploadProgress>();
let running = 0;
let chunkSize: number | null = null;

const publish = () => broadcast('uploads:progress', [...progress.values()]);

export const isUploading = () => running > 0 || queue.length > 0;

const getChunkSize = async () => {
    if (chunkSize) return chunkSize;

    const settings = await api<{ fileChunkSize?: number }>('/api/settings/public').catch(
        () => null,
    );

    chunkSize = (settings?.fileChunkSize ?? 25) * 1024 * 1024;
    return chunkSize;
};

export const resetUploadSettings = () => {
    chunkSize = null;
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
    const size = await getChunkSize();
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

    try {
        for (let index = 0; index < totalChunks; index++) {
            const offset = index * size;
            const chunk = new Blob([new Uint8Array(await readChunk(offset))], {
                type: job.mimeType,
            });

            result = await sendChunk(() => {
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

            progress.set(job.id, {
                id: job.id,
                name: job.name,
                progress: Math.round(((index + 1) / totalChunks) * 100),
            });

            publish();
        }
    } finally {
        await handle?.close();
    }

    if (!result?.url) throw new Error('The server did not return a link');
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
    if (!getStore().token) return Promise.reject(new Error('Log in to Fileship to upload'));

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
