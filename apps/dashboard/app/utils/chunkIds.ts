const BULK_LIMIT = 5000;

export const chunkIds = (ids: string[]) =>
    Array.from({ length: Math.ceil(ids.length / BULK_LIMIT) }, (_, index) =>
        ids.slice(index * BULK_LIMIT, (index + 1) * BULK_LIMIT),
    );
