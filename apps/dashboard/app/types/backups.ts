import type { BackupFrequency, BackupRunStatus } from '#shared/prisma/enums';

export type BackupData = {
    id: string;
    automatic: boolean;
    createdAt: Date;
    size: {
        raw: string;
        formatted: string;
    };
};

export type BackupScheduleData = {
    frequency: BackupFrequency;
    hour: number;
    weekday: number;
    timezone: string | null;
    keep: number;
    nextRunAt: string | null;
    lastRunAt: string | null;
    lastRunStatus: BackupRunStatus | null;
    backupLimit: number | null;
};
