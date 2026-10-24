import { app } from 'electron';

import { getLanguage, t } from './i18n';
import { getStore, getToken } from './store';

export class ApiError extends Error {
    public constructor(
        message: string,
        public status: number,
        public data: Record<string, unknown> | null = null,
    ) {
        super(message);
    }
}

export const isQuotaExceeded = (error: unknown) =>
    error instanceof ApiError && !!error.data?.quotaExceeded;

export const desktopUserAgent = () => `FileshipDesktop/${app.getVersion()} (Windows)`;

export const normalizeInstanceUrl = (input: string) => {
    const trimmed = input.trim();
    const url = new URL(/^https?:\/\//i.test(trimmed) ? trimmed : `https://${trimmed}`);

    return url.origin;
};

const readError = async (response: Response) => {
    const body = await response.json().catch(() => null);

    return new ApiError(
        (body?.message as string | undefined) ||
            t('app.requestFailed', { status: response.status }),
        response.status,
        body?.data ?? null,
    );
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
                'Accept-Language': getLanguage(),
                ...(token && { Authorization: `Bearer ${token}` }),
                ...headers,
            },
            signal: rest.signal ?? AbortSignal.timeout(timeout),
        });
    } catch (error: any) {
        throw new ApiError(
            error?.name === 'TimeoutError'
                ? t('app.serverTimeout')
                : t('app.serverUnreachable', { url: baseUrl }),
            0,
        );
    }

    if (!response.ok) throw await readError(response);

    const text = await response.text();
    return (text ? JSON.parse(text) : undefined) as T;
};

export const api = <T>(path: string, init: RequestInit & { timeout?: number } = {}) => {
    const { instanceUrl } = getStore();
    if (!instanceUrl) throw new ApiError(t('app.notConnected'), 401);

    return request<T>(instanceUrl, path, { ...init, token: getToken() });
};
