export type StatData = {
    users: {
        count: number;
        growth: number;
    };
    files: {
        count: number;
        growth: number;
    };
    storageUsed: {
        size: string;
        growth: number;
    };
    views: {
        count: number;
        growth: number;
        daily?: { labels: string[]; data: number[] };
    };
    storageByType?: { type: string; size: string; formatted: string }[];
};

export const useStats = () => {
    return useState<StatData | null>('stats', () => null);
};
