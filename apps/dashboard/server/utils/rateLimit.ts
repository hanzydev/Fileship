import type { H3Event } from 'h3';

const MAX_BUCKETS = 100_000;
const SWEEP_INTERVAL = 60_000;

const buckets = new Map<string, { count: number; resetAt: number }>();
let lastSweep = 0;

const sweep = (now: number) => {
    if (now - lastSweep < SWEEP_INTERVAL && buckets.size < MAX_BUCKETS) return;
    lastSweep = now;

    for (const [key, bucket] of buckets) {
        if (bucket.resetAt <= now || buckets.size >= MAX_BUCKETS) buckets.delete(key);
    }
};

const bucketOf = (key: string, windowMs: number) => {
    const now = Date.now();
    sweep(now);

    let bucket = buckets.get(key);

    if (!bucket || bucket.resetAt <= now) {
        bucket = { count: 0, resetAt: now + windowMs };
        buckets.delete(key);
        buckets.set(key, bucket);
    }

    return bucket;
};

export const rateLimit = async (key: string, limit: number, windowMs: number) => {
    const bucket = bucketOf(key, windowMs);

    if (bucket.count >= limit) {
        throw apiError(429, 'errorsCommon.tooManyAttempts');
    }

    bucket.count++;
};

export const limitPasswordAttempts = async (event: H3Event, resourceId: string, valid: boolean) => {
    const bucket = bucketOf(`password:${getClientIp(event)}:${resourceId}`, 10 * 60_000);

    if (bucket.count >= 20) {
        throw apiError(429, 'errorsCommon.tooManyAttempts');
    }

    if (!valid) bucket.count++;

    return valid;
};
