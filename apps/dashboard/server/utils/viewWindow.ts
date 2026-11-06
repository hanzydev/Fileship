import { createHash } from 'node:crypto';

const VIEW_WINDOW = 10 * 60 * 1_000;
const MAX_ENTRIES = 50_000;

const lastViews = new Map<string, number>();

export const claimView = (key: string) => {
    const id = createHash('sha1').update(key).digest('base64');
    const now = Date.now();
    const lastView = lastViews.get(id) ?? 0;

    if (now - lastView <= VIEW_WINDOW) return false;

    lastViews.delete(id);

    if (lastViews.size >= MAX_ENTRIES) {
        for (const [entry, at] of lastViews) {
            if (lastViews.size < MAX_ENTRIES && now - at <= VIEW_WINDOW) break;
            lastViews.delete(entry);
        }
    }

    lastViews.set(id, now);
    return true;
};
