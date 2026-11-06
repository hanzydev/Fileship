export const STORAGE_DOWNLOAD_CONCURRENCY = 6;

export const settleWithLimit = async <T, R>(
    items: T[],
    limit: number,
    fn: (item: T) => Promise<R>,
) => {
    const results: PromiseSettledResult<R>[] = Array.from({ length: items.length });
    let next = 0;

    const work = async () => {
        while (next < items.length) {
            const index = next++;

            results[index] = await fn(items[index]!).then(
                (value) => ({ status: 'fulfilled', value }) as const,
                (reason) => ({ status: 'rejected', reason }) as const,
            );
        }
    };

    await Promise.all(Array.from({ length: Math.min(limit, items.length) }, work));

    return results;
};
