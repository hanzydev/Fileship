export const MIN_FILESHIP_VERSION = '6.0.0';

export type Connection = {
    instanceUrl: string;
    token: string;
    language: string;
};

export type ConnectionInfo = {
    siteName: string;
    version: string;
    canUpload: boolean;
    canShorten: boolean;
};

export class ApiError extends Error {
    public constructor(
        public status: number,
        public data: Record<string, unknown> | null = null,
        public serverMessage = '',
    ) {
        super(serverMessage || `Request failed (${status})`);
    }
}

export class ExtensionError extends Error {
    public constructor(
        public key: string,
        public params: Record<string, string | number> = {},
    ) {
        super(key);
    }
}

export const normalizeInstanceUrl = (input: string) => {
    const trimmed = input.trim();
    if (!trimmed) return null;

    try {
        const hasScheme = /^[a-z][a-z\d+.-]*:\/\//i.test(trimmed);
        const url = new URL(hasScheme ? trimmed : `https://${trimmed}`);

        return ['http:', 'https:'].includes(url.protocol) ? url.origin : null;
    } catch {
        return null;
    }
};

export const originPattern = (url: string) => `${new URL(url).origin}/*`;

export const compareVersions = (a: string, b: string) => {
    const parse = (version: string) => version.split('.').map((part) => parseInt(part) || 0);
    const [left, right] = [parse(a), parse(b)];

    for (let index = 0; index < 3; index++) {
        const difference = (left[index] ?? 0) - (right[index] ?? 0);
        if (difference) return Math.sign(difference);
    }

    return 0;
};

export const request = async <T>(
    connection: Connection,
    path: string,
    init: RequestInit & { timeout?: number } = {},
): Promise<T> => {
    const { timeout = 30_000, headers, ...rest } = init;

    let response: Response;

    try {
        response = await fetch(`${connection.instanceUrl}${path}`, {
            credentials: 'omit',
            ...rest,
            headers: {
                'Accept-Language': connection.language,
                ...(connection.token && { Authorization: `Bearer ${connection.token}` }),
                ...headers,
            },
            signal: rest.signal ?? AbortSignal.timeout(timeout),
        });
    } catch (error: any) {
        throw new ApiError(0, { timeout: error?.name === 'TimeoutError' });
    }

    if (!response.ok) {
        const body = await response.json().catch(() => null);
        throw new ApiError(response.status, body?.data ?? null, body?.message ?? '');
    }

    const text = await response.text();
    return (text ? JSON.parse(text) : undefined) as T;
};

const probe = async (connection: Connection, path: string, init: RequestInit) => {
    try {
        await request(connection, path, { method: 'POST', ...init });
        return true;
    } catch (error) {
        if (!(error instanceof ApiError) || error.status === 0) throw error;
        if (error.status === 401) throw new ExtensionError('errors.invalidToken');
        return error.status !== 403;
    }
};

export const testConnection = async (connection: Connection): Promise<ConnectionInfo> => {
    const health = await request<{ version?: string }>(
        { ...connection, token: '' },
        '/api/healthz',
    ).catch((error) => {
        if (error instanceof ApiError && error.status === 503)
            throw new ExtensionError('errors.serverStarting');
        if (error instanceof ApiError && error.status === 0) throw error;
        return null;
    });

    if (!health?.version)
        throw new ExtensionError('errors.notFileship', { url: connection.instanceUrl });

    if (compareVersions(health.version, MIN_FILESHIP_VERSION) < 0)
        throw new ExtensionError('errors.serverTooOld', {
            version: health.version,
            minVersion: MIN_FILESHIP_VERSION,
        });

    const [canUpload, canShorten] = await Promise.all([
        probe(connection, '/api/files', { body: new FormData() }),
        probe(connection, '/api/links', {
            headers: { 'Content-Type': 'application/json' },
            body: '{}',
        }),
    ]);

    if (!canUpload && !canShorten) throw new ExtensionError('errors.noPermission');

    const settings = await request<{ siteName: string }>(connection, '/api/settings/public');

    return { siteName: settings.siteName, version: health.version, canUpload, canShorten };
};

export const shortenUrl = async (connection: Connection, url: string) => {
    const link = await request<{ shortUrl: string }>(connection, '/api/links', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ url }),
    });

    return link.shortUrl;
};
