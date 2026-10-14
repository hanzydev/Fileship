type StatGrowth = {
    count: number;
    growth: number;
};

export type StatData = {
    users: StatGrowth;
    notes: { count: number };
    folders: { count: number };
    links: { count: number };
    files: StatGrowth;
    storageUsed: {
        size: string;
        growth: number;
    };
    views: StatGrowth;
    byType: { type: string; count: number; size: string; formatted: string }[];
};

export const useStats = () => {
    return useState<StatData | null>('stats', () => null);
};
