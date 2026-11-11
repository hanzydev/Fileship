import { nanoid } from 'nanoid';

export interface FileSettings {
    fileNameType?: 'Random' | 'UUID' | 'Original';
    maxViews?: number;
    password?: string | null;
    expiration?: number | null;
    compression?: number;
    folder?: string | null;
    inboxPassword?: string;
    reportDuplicate?: boolean;
}

const CHUNK_REQUEST_TIMEOUT_MS = 120_000;
const CHUNK_REQUEST_ATTEMPTS = 3;

type ChunkResult = {
    ok: boolean;
    retryable: boolean;
    interrupted?: boolean;
    url?: string;
    error?: string;
    isQuotaExceeded?: boolean;
};

type UploadControl = {
    paused: boolean;
    canceled: boolean;
    interrupted: boolean;
    request: XMLHttpRequest | null;
    canAbort: () => boolean;
    wake: (() => void) | null;
};

const controls = new WeakMap<File, UploadControl>();

const controlOf = (file: File) => {
    const raw = toRaw(file);
    let control = controls.get(raw);

    if (!control) {
        control = {
            paused: false,
            canceled: false,
            interrupted: false,
            request: null,
            canAbort: () => false,
            wake: null,
        };
        controls.set(raw, control);
    }

    return control;
};

const interrupt = (control: UploadControl) => {
    if (!control.request || !control.canAbort()) return;

    control.interrupted = true;
    control.request.abort();
};

export const pauseUpload = (file: File) => {
    const control = controlOf(file);
    control.paused = true;
    interrupt(control);
};

export const resumeUpload = (file: File) => {
    const control = controlOf(file);
    control.paused = false;
    control.wake?.();
};

export const cancelUpload = (file: File) => {
    const control = controlOf(file);
    control.canceled = true;
    control.paused = false;
    interrupt(control);
    control.wake?.();
};

export const uploadFilesWithConcurrency = async <F extends File, T>(
    files: F[],
    upload: (file: F) => Promise<T>,
    concurrency = 3,
) => {
    const results = new Array<T>(files.length);
    let nextIndex = 0;

    const worker = async () => {
        while (nextIndex < files.length) {
            const index = nextIndex++;
            results[index] = await upload(files[index]!);
        }
    };

    await Promise.all(Array.from({ length: Math.min(concurrency, files.length) }, () => worker()));

    return results;
};

export const uploadFile = async (
    file: File,
    settings: FileSettings = {
        fileNameType: 'Random',
        maxViews: 0,
        password: null,
        expiration: null,
        compression: 0,
        folder: null,
    },
    uploadEndpoint = '/api/files',
) => {
    const uploadingFiles = useUploadingFiles();
    const { t } = useNuxtApp().$i18n;

    const fileChunkSize = usePublicSettings().value.fileChunkSize * 1024 * 1024;

    const chunks = Math.max(1, Math.ceil(file.size / fileChunkSize));
    const uploadingFile = uploadingFiles.value.find((f) => toRaw(f) === toRaw(file));
    const control = controlOf(file);

    if (!uploadingFile || control.canceled) return null;

    if (!uploadingFile.status) {
        uploadingFile.status = reactive({
            started: false,
            completed: false,
            paused: false,
            progress: {
                speed: 0,
                percent: 0,
                eta: 0,
            },
            error: null,
        });
    }

    uploadingFile.status!.started = true;
    uploadingFile.status!.completed = false;
    uploadingFile.status!.error = null;

    let startedAt = Date.now();
    const uploadId = nanoid();

    let uploadedFileUrl = '';

    for (let i = 0; i < chunks; i++) {
        const start = i * fileChunkSize;
        const end = Math.min(file.size, start + fileChunkSize);
        const chunk = file.slice(start, end);

        const formData = new FormData();

        formData.append('file', new Blob([chunk], { type: file.type }), file.name);
        formData.append('uploadId', uploadId);
        formData.append('currentChunk', (i + 1).toString());
        formData.append('totalChunks', chunks.toString());
        formData.append('chunkOffset', start.toString());

        if (settings.maxViews != null) formData.append('maxViews', settings.maxViews.toString());
        if (settings.fileNameType) formData.append('fileNameType', settings.fileNameType);
        if (settings.compression) formData.append('compression', settings.compression.toString());
        if (settings.password) formData.append('password', settings.password);
        if (settings.expiration) formData.append('expiration', settings.expiration.toString());
        if (settings.folder) formData.append('folderId', settings.folder);
        if (settings.inboxPassword) formData.append('inboxPassword', settings.inboxPassword);
        if (settings.reportDuplicate) formData.append('reportDuplicate', 'true');

        let result: ChunkResult = { ok: false, retryable: true };
        let chunkLoaded = 0;

        for (let attempt = 0; attempt < CHUNK_REQUEST_ATTEMPTS; attempt++) {
            if (control.paused) {
                const pausedAt = Date.now();
                await new Promise<void>((resolve) => (control.wake = resolve));
                control.wake = null;
                startedAt += Date.now() - pausedAt;
            }

            if (control.canceled) break;

            result = await new Promise<ChunkResult>((resolve) => {
                const req = new XMLHttpRequest();
                let settled = false;
                let attemptLoaded = 0;

                const canRetry = () => i < chunks - 1 || attemptLoaded < chunk.size;

                control.request = req;
                control.canAbort = canRetry;

                const settle = (value: ChunkResult) => {
                    if (settled) return;
                    settled = true;
                    resolve(value);
                };

                req.upload.addEventListener('progress', (e) => {
                    if (!e.lengthComputable) return;

                    attemptLoaded = e.loaded;
                    chunkLoaded = Math.max(chunkLoaded, e.loaded);
                    const loaded = start + chunkLoaded;
                    const elapsedSeconds = Math.max((Date.now() - startedAt) / 1000, 0.001);
                    const speed = loaded / elapsedSeconds;
                    const percent = file.size
                        ? clamp(Math.round((loaded / file.size) * 100), 0, 99)
                        : 0;
                    const eta = Math.max(0, Math.round((file.size - loaded) / speed));

                    uploadingFile.status!.progress = { speed, percent, eta };
                });

                req.addEventListener('load', () => {
                    let response: {
                        error?: boolean;
                        message?: string;
                        url?: string;
                        data?: { quotaExceeded?: boolean };
                    } = {};

                    if (req.responseText.length) {
                        try {
                            response = JSON.parse(req.responseText);
                        } catch {
                            settle({
                                ok: false,
                                retryable: req.status >= 500 && canRetry(),
                                error: t('upload.errors.invalidResponse', {
                                    status: req.status,
                                }),
                            });
                            return;
                        }
                    }

                    if (req.status < 200 || req.status >= 300 || response.error) {
                        settle({
                            ok: false,
                            retryable: (req.status === 0 || req.status >= 500) && canRetry(),
                            error:
                                response.message ||
                                t('upload.errors.failedStatus', { status: req.status }),
                            isQuotaExceeded: !!response.data?.quotaExceeded,
                        });
                        return;
                    }

                    settle({ ok: true, retryable: false, url: response.url });
                });
                req.addEventListener('error', () =>
                    settle({
                        ok: false,
                        retryable: canRetry(),
                        error: t('upload.errors.network'),
                    }),
                );
                req.addEventListener('timeout', () =>
                    settle({
                        ok: false,
                        retryable: canRetry(),
                        error: t('upload.errors.timeout'),
                    }),
                );
                req.addEventListener('abort', () => {
                    if (control.interrupted) {
                        control.interrupted = false;
                        settle({ ok: false, retryable: true, interrupted: true });
                        return;
                    }

                    settle({
                        ok: false,
                        retryable: canRetry(),
                        error: t('upload.errors.aborted'),
                    });
                });

                req.open('POST', uploadEndpoint);
                req.timeout = CHUNK_REQUEST_TIMEOUT_MS;
                req.send(formData);
            });

            control.request = null;

            if (result.interrupted) {
                attempt--;
                continue;
            }

            if (result.ok || !result.retryable) break;
        }

        if (control.canceled && !result.ok) {
            if (uploadEndpoint === '/api/files') {
                $fetch(`/api/files/uploads/${uploadId}`, { method: 'DELETE' }).catch(() => null);
            }

            return null;
        }

        if (!result.ok) {
            uploadingFile.status!.error = result.error || t('upload.errors.failed');

            if (result.isQuotaExceeded && uploadEndpoint === '/api/files') {
                useNuxtApp().$toast.error(t('storage.full'), {
                    id: 'storage-full',
                    description: t('storage.fullDescription'),
                    action: {
                        label: t('storage.manage'),
                        onClick: () => navigateTo('/dashboard/settings/storage'),
                    },
                });
            }

            return false;
        }

        if (result.url) uploadedFileUrl = result.url;
    }

    uploadingFile.status!.completed = true;
    uploadingFile.status!.progress = { ...uploadingFile.status!.progress, percent: 100, eta: 0 };

    return uploadedFileUrl;
};
