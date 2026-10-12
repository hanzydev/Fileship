export const rateLimit = async (key: string, limit: number, windowMs: number) => {
    const storage = useStorage('cache');
    const now = Date.now();
    const entry = await storage.getItem<{ count: number; resetAt: number }>(`rateLimit:${key}`);
    const current = entry && entry.resetAt > now ? entry : { count: 0, resetAt: now + windowMs };

    if (current.count >= limit) {
        throw createError({
            status: 429,
            message: 'Too many attempts, try again later',
        });
    }

    await storage.setItem(`rateLimit:${key}`, { ...current, count: current.count + 1 });
};
