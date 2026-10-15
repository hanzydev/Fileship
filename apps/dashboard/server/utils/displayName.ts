export const toDisplayName = (name: string, fallback: string) =>
    name
        .replace(/[\\/:*?"<>|\p{Cc}]/gu, '')
        .trim()
        .slice(0, 255) || fallback;
