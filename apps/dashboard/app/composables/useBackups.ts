export type BackupData = {
    id: string;
    createdAt: Date;
    size: {
        raw: number;
        formatted: string;
    };
};
