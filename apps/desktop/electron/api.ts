import { app } from 'electron';

import { getStore, getToken } from './store';

export class ApiError extends Error {
    public constructor(
        message: string,
        public status: number,
    ) {
        super(message);
    }
}

export const desktopUserAgent = () => `FileshipDesktop/${app.getVersion()} (Windows)`;

export const normalizeInstanceUrl = (input: string) => {
    const trimmed = input.trim();
    const url = new URL(/^https?:\/\//i.test(trimmed) ? trimmed : `https://${trimmed}`);

    return url.origin;
};

const readError = async (response: Response) => {
    const body = await response.json().catch(() => null);
    return (body?.message as string | undefined) || `Request failed (${response.status})`;
};

export const request = async <T>(
    baseUrl: string,
    path: string,
    init: RequestInit & { token?: string | null; timeout?: number } = {},
): Promise<T> => {
    const { token, timeout = 30_000, headers, ...rest } = init;

    let response: Response;

    try {
        response = await fetch(`${baseUrl}${path}`, {
            ...rest,
            headers: {
                'User-Agent': desktopUserAgent(),
                ...(token && { Authorization: `Bearer ${token}` }),
                ...headers,
            },
            signal: rest.signal ?? AbortSignal.timeout(timeout),
        });
    } catch (error: any) {
        throw new ApiError(
            error?.name === 'TimeoutError'
                ? 'The server took too long to respond'
                : `Could not reach ${baseUrl}`,
            0,
        );
    }

    if (!response.ok) throw new ApiError(await readError(response), response.status);

    const text = await response.text();
    return (text ? JSON.parse(text) : undefined) as T;
};

export const api = <T>(path: string, init: RequestInit & { timeout?: number } = {}) => {
    const { instanceUrl } = getStore();
    if (!instanceUrl) throw new ApiError('Not connected to a Fileship server', 401);

    return request<T>(instanceUrl, path, { ...init, token: getToken() });
};
