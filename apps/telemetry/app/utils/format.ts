import { filesize } from 'filesize';

const numberFormat = new Intl.NumberFormat('en-US');

export const formatNumber = (value: number) => numberFormat.format(value);

export const formatBytes = (value: number) => filesize(value) as string;

export const formatShare = (count: number, total: number) =>
    total ? `${Math.round((count / total) * 100)}%` : '0%';

export const toDonutItems = (
    counts: Record<string, number> | undefined,
    format?: (value: number) => string,
) =>
    Object.entries(counts ?? {}).map(([label, value]) => ({
        label,
        value,
        formatted: format?.(value),
    }));
