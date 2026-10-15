export type BackupData = {
    id: string;
    createdAt: Date;
    size: {
        raw: string;
        formatted: string;
    };
};
