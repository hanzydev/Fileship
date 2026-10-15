import type { H3Event } from 'h3';

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

export const limitPasswordAttempts = async (event: H3Event, resourceId: string, valid: boolean) => {
    const storage = useStorage('cache');
    const key = `rateLimit:password:${getClientIp(event)}:${resourceId}`;
    const now = Date.now();
    const entry = await storage.getItem<{ count: number; resetAt: number }>(key);
    const current = entry && entry.resetAt > now ? entry : { count: 0, resetAt: now + 10 * 60_000 };

    if (current.count >= 20) {
        throw createError({
            status: 429,
            message: 'Too many attempts, try again later',
        });
    }

    if (!valid) await storage.setItem(key, { ...current, count: current.count + 1 });

    return valid;
};
