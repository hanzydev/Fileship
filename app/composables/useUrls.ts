import type { Url } from '@prisma/client';

export type UrlData = Url & { views: { total: number; today: number } };

export const useUrls = () => {
    return useState<UrlData[]>('urls', () => []);
};
