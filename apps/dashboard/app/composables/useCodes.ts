import type { Code } from '@prisma/client';

export type CodeData = Code & {
    views: { total: number; today: number };
    url: string;
};

export const useCodes = () => {
    return useState<CodeData[]>('codes', () => []);
};
