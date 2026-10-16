import type { H3Event } from 'h3';

const MAX_BODY_SIZE = 64 * 1024;

const tooLargeError = () =>
    createError({
        status: 413,
        message: 'Report is too large',
    });

export const readJsonBody = async (event: H3Event): Promise<unknown> => {
    if (Number(getRequestHeader(event, 'content-length') || 0) > MAX_BODY_SIZE) {
        throw tooLargeError();
    }

    const raw = await readRawBody(event);
    if (raw && raw.length > MAX_BODY_SIZE) throw tooLargeError();

    try {
        return raw ? JSON.parse(raw) : undefined;
    } catch {
        return undefined;
    }
};
