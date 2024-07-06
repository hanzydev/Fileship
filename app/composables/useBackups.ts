export type BackupData = {
    id: string;
    createdAt: Date;
    size: {
        raw: number;
        formatted: string;
    };
};

export const useBackups = () => {
    return useState<BackupData[]>('backups', () => []);
};
