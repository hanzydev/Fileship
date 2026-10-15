const VIEW_WINDOW = 10 * 60 * 1_000;
const PRUNE_SIZE = 20_000;

const lastViews = new Map<string, number>();

export const claimView = (key: string) => {
    const now = Date.now();
    const lastView = lastViews.get(key) ?? 0;

    if (now - lastView <= VIEW_WINDOW) return false;

    if (lastViews.size >= PRUNE_SIZE) {
        for (const [entry, at] of lastViews) {
            if (now - at > VIEW_WINDOW) lastViews.delete(entry);
        }
    }

    lastViews.set(key, now);
    return true;
};
