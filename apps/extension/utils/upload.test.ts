import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';

const MB = 1024 * 1024;

type Handler = (url: URL, init: RequestInit) => Response | Promise<Response>;

let instances = 0;

const createConnection = (): Connection => ({
    instanceUrl: `https://files-${++instances}.test`,
    token: 'fs_test',
    language: 'en',
});

const json = (body: unknown, status = 200) =>
    new Response(JSON.stringify(body), {
        status,
        headers: { 'Content-Type': 'application/json' },
    });

const serve = (handlers: Record<string, Handler>) => {
    const calls: { url: URL; init: RequestInit }[] = [];

    vi.stubGlobal(
        'fetch',
        vi.fn(async (input: string, init: RequestInit = {}) => {
            const url = new URL(input);
            calls.push({ url, init });

            const handler = handlers[`${init.method ?? 'GET'} ${url.pathname}`];
            return handler ? handler(url, init) : json({ message: 'Not found' }, 404);
        }),
    );

    return calls;
};

const settings = (fileChunkSize: number, uploadConcurrency: number) => () =>
    json({ siteName: 'Test', fileChunkSize, uploadConcurrency });

const readChunk = (init: RequestInit) => {
    const form = init.body as FormData;
    const file = form.get('file') as File;

    return {
        uploadId: form.get('uploadId'),
        currentChunk: Number(form.get('currentChunk')),
        totalChunks: Number(form.get('totalChunks')),
        chunkOffset: Number(form.get('chunkOffset')),
        fileNameType: form.get('fileNameType'),
        expiration: form.get('expiration'),
        name: file.name,
        size: file.size,
    };
};

const uploadServer = (totalChunks: number) => {
    let received = 0;
    let inFlight = 0;
    let maxInFlight = 0;

    const handler: Handler = async () => {
        inFlight++;
        maxInFlight = Math.max(maxInFlight, inFlight);
        await new Promise((resolve) => setTimeout(resolve, 10));
        inFlight--;

        return ++received === totalChunks
            ? json({
                  id: '1',
                  name: 'a.bin',
                  fileName: 'abc.bin',
                  url: 'https://files.test/u/abc.bin',
              })
            : new Response(null, { status: 200 });
    };

    return { handler, getMaxInFlight: () => maxInFlight };
};

beforeEach(() => {
    vi.spyOn(fakeBrowser.i18n, 'getUILanguage').mockReturnValue('en');
});

afterEach(() => {
    vi.unstubAllGlobals();
    vi.restoreAllMocks();
});

describe('uploadBlob', () => {
    it('splits files by the server chunk size and sends chunks in parallel', async () => {
        const server = uploadServer(3);
        const calls = serve({
            'GET /api/settings/public': settings(5, 2),
            'POST /api/files': server.handler,
        });

        const progress: number[] = [];
        const blob = new Blob([new Uint8Array(12 * MB)], { type: 'application/octet-stream' });
        const file = await uploadBlob(
            createConnection(),
            { blob, name: 'a.bin' },
            { fileNameType: 'UUID', expiration: 3_600_000 },
            (sent) => progress.push(sent),
        );

        const chunks = calls
            .filter((call) => call.url.pathname === '/api/files')
            .map((call) => readChunk(call.init))
            .sort((a, b) => a.currentChunk - b.currentChunk);

        expect(file.url).toBe('https://files.test/u/abc.bin');
        expect(server.getMaxInFlight()).toBe(2);
        expect(new Set(chunks.map((chunk) => chunk.uploadId)).size).toBe(1);
        expect(chunks[0]!.uploadId).toMatch(/^[\w-]{16}$/);
        expect(chunks.map((chunk) => [chunk.currentChunk, chunk.chunkOffset, chunk.size])).toEqual([
            [1, 0, 5 * MB],
            [2, 5 * MB, 5 * MB],
            [3, 10 * MB, 2 * MB],
        ]);
        expect(chunks.every((chunk) => chunk.totalChunks === 3)).toBe(true);
        expect(chunks.every((chunk) => chunk.fileNameType === 'UUID')).toBe(true);
        expect(chunks.every((chunk) => chunk.expiration === '3600000')).toBe(true);
        expect(chunks.every((chunk) => chunk.name === 'a.bin')).toBe(true);
        expect(progress.at(-1)).toBe(12 * MB);

        const upload = calls.find((call) => call.url.pathname === '/api/files')!;
        expect(upload.init.credentials).toBe('omit');
        expect(new Headers(upload.init.headers).get('Authorization')).toBe('Bearer fs_test');
    });

    it('sends one chunk at a time when the storage is sequential', async () => {
        const server = uploadServer(4);
        serve({
            'GET /api/settings/public': settings(5, 1),
            'POST /api/files': server.handler,
        });

        await uploadBlob(
            createConnection(),
            { blob: new Blob([new Uint8Array(16 * MB)]), name: 'a.bin' },
            { fileNameType: 'Random', expiration: 0 },
        );

        expect(server.getMaxInFlight()).toBe(1);
    });

    it('falls back to 25 MB chunks one at a time when the settings cannot be read', async () => {
        const server = uploadServer(2);
        const calls = serve({
            'GET /api/settings/public': () => json({}, 500),
            'POST /api/files': server.handler,
        });

        await uploadBlob(
            createConnection(),
            { blob: new Blob([new Uint8Array(30 * MB)]), name: 'a.bin' },
            { fileNameType: 'Random', expiration: 0 },
        );

        const sizes = calls
            .filter((call) => call.url.pathname === '/api/files')
            .map((call) => readChunk(call.init).size);

        expect(sizes).toEqual([25 * MB, 5 * MB]);
        expect(server.getMaxInFlight()).toBe(1);
    });

    it('retries a chunk after a server error', async () => {
        let attempts = 0;
        serve({
            'GET /api/settings/public': settings(5, 4),
            'POST /api/files': () =>
                ++attempts === 1
                    ? json({ message: 'Bad gateway' }, 502)
                    : json({
                          id: '1',
                          name: 'a.png',
                          fileName: 'a.png',
                          url: 'https://files.test/u/a.png',
                      }),
        });

        const file = await uploadBlob(
            createConnection(),
            { blob: new Blob(['image'], { type: 'image/png' }), name: 'a.png' },
            { fileNameType: 'Original', expiration: 0 },
        );

        expect(attempts).toBe(2);
        expect(file.url).toBe('https://files.test/u/a.png');
    });

    it('stops when the storage is full', async () => {
        let attempts = 0;
        serve({
            'GET /api/settings/public': settings(5, 4),
            'POST /api/files': () => {
                attempts++;
                return json({ message: 'Storage full', data: { quotaExceeded: true } }, 413);
            },
        });

        const error = await uploadBlob(
            createConnection(),
            { blob: new Blob(['data']), name: 'a.txt' },
            { fileNameType: 'Random', expiration: 0 },
        ).catch((error) => error);

        expect(error).toBeInstanceOf(ApiError);
        expect(attempts).toBe(1);
        expect(describeError(error)).toBe('Your storage is full.');
    });

    it('does not retry an invalid token', async () => {
        let attempts = 0;
        serve({
            'GET /api/settings/public': settings(5, 4),
            'POST /api/files': () => {
                attempts++;
                return json({ message: 'Unauthorized' }, 401);
            },
        });

        const error = await uploadBlob(
            createConnection(),
            { blob: new Blob(['data']), name: 'a.txt' },
            { fileNameType: 'Random', expiration: 0 },
        ).catch((error) => error);

        expect(attempts).toBe(1);
        expect(describeError(error)).toBe(
            'The upload token is not valid. Copy it again from Settings → Integrations.',
        );
    });

    it('fails when the server returns no link', async () => {
        serve({
            'GET /api/settings/public': settings(5, 4),
            'POST /api/files': () => new Response(null, { status: 200 }),
        });

        const error = await uploadBlob(
            createConnection(),
            { blob: new Blob(['data']), name: 'a.txt' },
            { fileNameType: 'Random', expiration: 0 },
        ).catch((error) => error);

        expect(describeError(error)).toBe('The server did not return a link.');
    });
});

describe('shortenUrl', () => {
    it('posts the link and returns the short link', async () => {
        const calls = serve({
            'POST /api/links': () => json({ code: 'abc', shortUrl: 'https://files.test/s/abc' }),
        });

        const shortUrl = await shortenUrl(createConnection(), 'https://example.com/a?b=c');

        expect(shortUrl).toBe('https://files.test/s/abc');
        expect(JSON.parse(calls[0]!.init.body as string)).toEqual({
            url: 'https://example.com/a?b=c',
        });
    });

    it('reports a token that cannot shorten links', async () => {
        serve({ 'POST /api/links': () => json({ message: 'Forbidden' }, 403) });

        const error = await shortenUrl(createConnection(), 'https://example.com').catch(
            (error) => error,
        );

        expect(describeError(error, 'link')).toBe(
            'Your account cannot shorten links on this server.',
        );
    });
});

describe('testConnection', () => {
    it('reports what the token can do', async () => {
        const calls = serve({
            'GET /api/healthz': () => json({ status: 'ok', version: '6.1.0', ready: true }),
            'GET /api/settings/public': settings(25, 4),
            'POST /api/files': () => json({ message: 'Missing file' }, 400),
            'POST /api/links': () => json({ message: 'Forbidden' }, 403),
        });

        const info = await testConnection(createConnection());

        expect(info).toEqual({
            siteName: 'Test',
            version: '6.1.0',
            canUpload: true,
            canShorten: false,
        });

        const health = calls.find((call) => call.url.pathname === '/api/healthz')!;
        expect(new Headers(health.init.headers).has('Authorization')).toBe(false);
    });

    it('rejects an invalid token', async () => {
        serve({
            'GET /api/healthz': () => json({ version: '6.0.0' }),
            'POST /api/files': () => json({ message: 'Unauthorized' }, 401),
            'POST /api/links': () => json({ message: 'Unauthorized' }, 401),
        });

        const error = await testConnection(createConnection()).catch((error) => error);

        expect(error).toBeInstanceOf(ExtensionError);
        expect(error.key).toBe('errors.invalidToken');
    });

    it('rejects servers older than Fileship 6', async () => {
        serve({ 'GET /api/healthz': () => json({ version: '5.4.4' }) });

        const error = await testConnection(createConnection()).catch((error) => error);

        expect(describeError(error)).toBe(
            'This server runs Fileship v5.4.4. The extension needs v6.0.0 or later.',
        );
    });

    it('rejects addresses that are not a Fileship server', async () => {
        serve({ 'GET /api/healthz': () => new Response('<html></html>', { status: 200 }) });

        const connection = createConnection();
        const error = await testConnection(connection).catch((error) => error);

        expect(describeError(error)).toBe(
            `No Fileship server was found at ${connection.instanceUrl}.`,
        );
    });

    it('reports a server that cannot be reached', async () => {
        vi.stubGlobal(
            'fetch',
            vi.fn(() => Promise.reject(new TypeError('Failed to fetch'))),
        );

        const error = await testConnection(createConnection()).catch((error) => error);

        expect(describeError(error)).toBe('Could not reach your Fileship server.');
    });
});

describe('normalizeInstanceUrl', () => {
    it('keeps only the origin and adds https', () => {
        expect(normalizeInstanceUrl(' files.example.com/dashboard ')).toBe(
            'https://files.example.com',
        );
        expect(normalizeInstanceUrl('http://localhost:3000/')).toBe('http://localhost:3000');
        expect(normalizeInstanceUrl('ftp://files.example.com')).toBeNull();
        expect(normalizeInstanceUrl('')).toBeNull();
    });
});
