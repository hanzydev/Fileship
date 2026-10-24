const MAX_ATTEMPTS = 3;
const LIMITS_TTL_MS = 5 * 60 * 1000;

export type UploadOptions = {
    fileNameType: FileNameType;
    expiration: number;
};

export type UploadedFile = {
    id: string;
    name: string;
    fileName: string;
    url: string;
    directUrl: string;
};

export type UploadLimits = {
    chunkSize: number;
    concurrency: number;
};

const limits = new Map<string, UploadLimits & { at: number }>();

export const getUploadLimits = async (connection: Connection): Promise<UploadLimits> => {
    const cached = limits.get(connection.instanceUrl);
    if (cached && Date.now() - cached.at < LIMITS_TTL_MS) return cached;

    const settings = await request<{ fileChunkSize?: number; uploadConcurrency?: number }>(
        connection,
        '/api/settings/public',
    ).catch(() => null);

    const next = {
        chunkSize: (settings?.fileChunkSize ?? 25) * 1024 * 1024,
        concurrency: Math.max(1, settings?.uploadConcurrency ?? 1),
        at: Date.now(),
    };

    if (settings) limits.set(connection.instanceUrl, next);
    return next;
};

const createUploadId = () =>
    btoa(String.fromCharCode(...crypto.getRandomValues(new Uint8Array(12))))
        .replaceAll('+', '-')
        .replaceAll('/', '_');

const sendChunk = async (connection: Connection, form: () => FormData) => {
    for (let attempt = 1; ; attempt++) {
        try {
            return await request<UploadedFile | undefined>(connection, '/api/files', {
                method: 'POST',
                body: form(),
                timeout: 120_000,
            });
        } catch (error) {
            const isRetryable =
                error instanceof ApiError && (error.status === 0 || error.status >= 500);

            if (!isRetryable || attempt >= MAX_ATTEMPTS) throw error;

            await new Promise((resolve) => setTimeout(resolve, attempt * 1000));
        }
    }
};

export const uploadBlob = async (
    connection: Connection,
    file: { blob: Blob; name: string },
    options: UploadOptions,
    onProgress?: (sent: number, total: number) => void,
) => {
    const { chunkSize, concurrency } = await getUploadLimits(connection);
    const uploadId = createUploadId();
    const total = file.blob.size;
    const totalChunks = Math.max(1, Math.ceil(total / chunkSize));

    let result: UploadedFile | undefined;
    let next = 0;
    let sent = 0;
    let failure: unknown = null;

    const sendPart = async (index: number) => {
        const offset = index * chunkSize;
        const chunk = file.blob.slice(offset, offset + chunkSize, file.blob.type);

        const response = await sendChunk(connection, () => {
            const form = new FormData();

            form.append('uploadId', uploadId);
            form.append('currentChunk', String(index + 1));
            form.append('totalChunks', String(totalChunks));
            form.append('chunkOffset', String(offset));
            form.append('fileNameType', options.fileNameType);
            if (options.expiration) form.append('expiration', String(options.expiration));
            form.append('file', chunk, file.name);

            return form;
        });

        if (response) result = response;
        sent += chunk.size;
        onProgress?.(sent, total);
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

    await Promise.all(Array.from({ length: Math.min(concurrency, totalChunks) }, worker));

    if (failure) throw failure;
    if (!result?.url) throw new ExtensionError('errors.noLink');

    return result;
};
