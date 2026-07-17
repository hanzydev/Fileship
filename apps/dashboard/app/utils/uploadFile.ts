interface FileSettings {
    fileNameType?: 'Random' | 'UUID' | 'Original';
    maxViews?: number;
    password?: string | null;
    expiration?: number | null;
    compression?: number;
    folder?: string | null;
    inboxPassword?: string;
}

const CHUNK_REQUEST_TIMEOUT_MS = 120_000;
const CHUNK_REQUEST_ATTEMPTS = 3;

type ChunkResult = {
    ok: boolean;
    retryable: boolean;
    url?: string;
    error?: string;
};

export const uploadFilesWithConcurrency = async <T>(
    files: File[],
    upload: (file: File) => Promise<T>,
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
    let {
        public: { fileChunkSize },
    } = useRuntimeConfig();
    const uploadingFiles = useUploadingFiles();

    fileChunkSize = (+fileChunkSize || 25) * 1024 * 1024;

    const chunks = Math.ceil(file.size / fileChunkSize);
    const uploadingFile = uploadingFiles.value.find((f) => f.name === file.name);

    if (!uploadingFile) return;

    if (!uploadingFile.status) {
        uploadingFile.status = reactive({
            started: false,
            completed: false,
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

    const startedAt = Date.now();
    let uploadedFileUrl = '';

    for (let i = 0; i < chunks; i++) {
        const start = i * fileChunkSize;
        const end = Math.min(file.size, start + fileChunkSize);
        const chunk = file.slice(start, end);

        const formData = new FormData();

        formData.append('file', new Blob([chunk], { type: file.type }), file.name);
        formData.append('currentChunk', (i + 1).toString());
        formData.append('totalChunks', chunks.toString());
        formData.append('chunkOffset', start.toString());

        if ('maxViews' in settings) formData.append('maxViews', settings.maxViews!.toString());
        if (settings.fileNameType) formData.append('fileNameType', settings.fileNameType);
        if (settings.compression) formData.append('compression', settings.compression.toString());
        if (settings.password) formData.append('password', settings.password);
        if (settings.expiration) formData.append('expiration', settings.expiration.toString());
        if (settings.folder) formData.append('folderId', settings.folder);
        if (settings.inboxPassword) formData.append('inboxPassword', settings.inboxPassword);

        let result: ChunkResult = { ok: false, retryable: true };

        for (let attempt = 0; attempt < CHUNK_REQUEST_ATTEMPTS; attempt++) {
            result = await new Promise<ChunkResult>((resolve) => {
                const req = new XMLHttpRequest();
                let settled = false;
                let attemptLoaded = 0;

                const canRetry = () => i < chunks - 1 || attemptLoaded < chunk.size;

                const settle = (value: ChunkResult) => {
                    if (settled) return;
                    settled = true;
                    resolve(value);
                };

                req.upload.addEventListener('progress', (e) => {
                    if (!e.lengthComputable) return;

                    attemptLoaded = e.loaded;
                    const loaded = start + e.loaded;
                    const elapsedSeconds = Math.max((Date.now() - startedAt) / 1000, 0.001);
                    const speed = loaded / elapsedSeconds;
                    const percent = clamp(Math.round((loaded / file.size) * 100), 0, 99);
                    const eta = Math.max(0, Math.round((file.size - loaded) / speed));

                    uploadingFile.status!.progress = { speed, percent, eta };
                });

                req.addEventListener('load', () => {
                    let response: { error?: boolean; message?: string; url?: string } = {};

                    if (req.responseText.length) {
                        try {
                            response = JSON.parse(req.responseText);
                        } catch {
                            settle({
                                ok: false,
                                retryable: req.status >= 500 && canRetry(),
                                error: `Upload failed with an invalid server response (${req.status})`,
                            });
                            return;
                        }
                    }

                    if (req.status < 200 || req.status >= 300 || response.error) {
                        settle({
                            ok: false,
                            retryable: (req.status === 0 || req.status >= 500) && canRetry(),
                            error: response.message || `Upload failed (${req.status})`,
                        });
                        return;
                    }

                    settle({ ok: true, retryable: false, url: response.url });
                });
                req.addEventListener('error', () =>
                    settle({
                        ok: false,
                        retryable: canRetry(),
                        error: 'Network error during upload',
                    }),
                );
                req.addEventListener('timeout', () =>
                    settle({
                        ok: false,
                        retryable: canRetry(),
                        error: 'Upload request timed out',
                    }),
                );
                req.addEventListener('abort', () =>
                    settle({
                        ok: false,
                        retryable: canRetry(),
                        error: 'Upload request was aborted',
                    }),
                );

                req.open('POST', uploadEndpoint);
                req.timeout = CHUNK_REQUEST_TIMEOUT_MS;
                req.send(formData);
            });

            if (result.ok || !result.retryable) break;
        }

        if (!result.ok) {
            uploadingFile.status!.error = result.error || 'Upload failed';
            return false;
        }

        if (result.url) uploadedFileUrl = result.url;
    }

    uploadingFile.status!.completed = true;
    uploadingFile.status!.progress = { ...uploadingFile.status!.progress, percent: 100, eta: 0 };

    return uploadedFileUrl;
};
