import type { H3Event } from 'h3';

const MAX_BODY_SIZE = 64 * 1024;

export const assertBodySize = (event: H3Event) => {
    if (Number(getRequestHeader(event, 'content-length') || 0) > MAX_BODY_SIZE) {
        throw createError({
            status: 413,
            message: 'Report is too large',
        });
    }
};
