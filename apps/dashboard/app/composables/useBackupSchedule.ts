import type { BackupScheduleData } from '~/types/backups';

export const useBackupSchedule = () =>
    useState<BackupScheduleData | null>('backupSchedule', () => null);
