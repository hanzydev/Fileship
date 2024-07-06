import type { Code } from '@prisma/client';

export type CodeData = Code & { views: { total: number; today: number } };

export const useCodes = () => {
    return useState<CodeData[]>('codes', () => []);
};
