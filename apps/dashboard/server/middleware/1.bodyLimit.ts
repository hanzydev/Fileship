const MB = 1024 * 1024;

const STREAMED_ROUTES = [
    /^\/api\/files$/,
    /^\/api\/inbox\/[^/]+\/upload$/,
    /^\/api\/users\/@me\/backups\/upload$/,
    /^\/api\/storages\/import\/apps\/source$/,
];

const bodyLimitOf = (method: string, path: string) => {
    if (path === '/api/ai/ocr') return 21 * MB;

    if (method === 'PATCH' && /^\/api\/users\/[^/]+$/.test(path))
        return Math.ceil((getSettingsSync().avatarMaxSize * MB * 4) / 3) + MB;

    return MB;
};

export default defineEventHandler((event) => {
    if (!['POST', 'PUT', 'PATCH', 'DELETE'].includes(event.method)) return;

    const path = event.path.split('?')[0]!;
    if (!path.startsWith('/api/') || STREAMED_ROUTES.some((route) => route.test(path))) return;

    const length = getRequestHeader(event, 'content-length');

    if (length === undefined) {
        if (getRequestHeader(event, 'transfer-encoding'))
            throw apiError(411, 'errorsSystem.lengthRequired');
        return;
    }

    if (+length > bodyLimitOf(event.method, path))
        throw apiError(413, 'errorsSystem.requestTooLarge');
});
