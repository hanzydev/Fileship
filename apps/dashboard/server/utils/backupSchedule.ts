import dayjs from 'dayjs';
import timezone from 'dayjs/plugin/timezone.js';
import utc from 'dayjs/plugin/utc.js';

import { BackupFrequency, BackupRunStatus } from '#shared/prisma/enums';
import type { BackupScheduleModel, UserModel } from '#shared/prisma/models';

dayjs.extend(utc);
dayjs.extend(timezone);

type ScheduleTime = Pick<BackupScheduleModel, 'frequency' | 'hour' | 'weekday' | 'timezone'>;

const DEFAULT_BACKUP_SCHEDULE = {
    frequency: BackupFrequency.Off,
    hour: 3,
    weekday: 1,
    timezone: null,
    keep: 7,
    nextRunAt: null,
    lastRunAt: null,
    lastRunStatus: null,
};

export const isValidTimezone = (timeZone: string) => {
    try {
        new Intl.DateTimeFormat('en-US', { timeZone });
        return true;
    } catch {
        return false;
    }
};

export const nextBackupRun = (schedule: ScheduleTime, after = new Date()) => {
    if (schedule.frequency === BackupFrequency.Off) return null;

    const today = dayjs.utc(dayjs(after).tz(schedule.timezone).format('YYYY-MM-DD'));

    for (let i = 0; ; i++) {
        const day = today.add(i, 'day').hour(schedule.hour);
        if (schedule.frequency === BackupFrequency.Weekly && day.day() !== schedule.weekday)
            continue;

        const run = dayjs.tz(day.format('YYYY-MM-DD HH:mm'), schedule.timezone).toDate();
        if (run > after) return run;
    }
};

export const serializeBackupSchedule = (
    schedule: BackupScheduleModel | null,
    owner: NonNullable<PartialUser> & { limits: any },
) => {
    const { userId: _, ...data } = schedule ?? { userId: null, ...DEFAULT_BACKUP_SCHEDULE };
    const backupLimit = backupLimitOf(owner);

    return { ...data, backupLimit: Number.isFinite(backupLimit) ? backupLimit : null };
};

const claimDueBackupSchedule = async () => {
    for (;;) {
        const schedule = await prisma.backupSchedule.findFirst({
            where: { nextRunAt: { lte: new Date() }, user: { backupRestoreState: null } },
            orderBy: { nextRunAt: 'asc' },
            include: { user: true },
        });

        if (!schedule) return null;

        const { count } = await prisma.backupSchedule.updateMany({
            where: { userId: schedule.userId, nextRunAt: schedule.nextRunAt },
            data: { nextRunAt: nextBackupRun(schedule) },
        });

        if (count) return schedule;
    }
};

const rotateBackups = async (user: UserModel, keep: number) => {
    const stale = await prisma.backup.findMany({
        where: { userId: user.id, automatic: true },
        orderBy: { createdAt: 'desc' },
        skip: keep,
    });

    await removeBackupData(stale);
    await prisma.backup.deleteMany({ where: { id: { in: stale.map((backup) => backup.id) } } });

    for (const backup of stale) {
        await createLog(null, {
            action: 'Delete Backup',
            message: `Deleted automatic backup ${backup.id}`,
            user,
        });

        sendToUser(user.id, 'backup:delete', backup.id);
    }
};

const runBackupSchedule = async ({ user, keep }: BackupScheduleModel & { user: UserModel }) => {
    const limit = backupLimitOf(user);
    const manual = await prisma.backup.count({ where: { userId: user.id, automatic: false } });

    if (manual >= limit) {
        await createLog(null, {
            action: 'Create Backup Failed',
            message: 'Skipped the automatic backup because the backup limit is reached',
            user,
        });

        return BackupRunStatus.LimitReached;
    }

    try {
        const backup = await createBackup(user, true);

        await createLog(null, {
            action: 'Create Backup',
            message: `Created automatic backup ${backup.id}`,
            user,
        });

        await rotateBackups(user, Math.min(keep, limit - manual));

        return BackupRunStatus.Created;
    } catch {
        await createLog(null, {
            action: 'Create Backup Failed',
            message: 'Failed to create the automatic backup',
            user,
        }).catch(() => null);

        return BackupRunStatus.Failed;
    }
};

export const runDueBackupSchedules = async () => {
    let runs = 0;

    for (;;) {
        const schedule = await claimDueBackupSchedule();
        if (!schedule) return runs;

        const lastRunStatus = await runBackupSchedule(schedule);

        const updated = await prisma.backupSchedule
            .update({
                where: { userId: schedule.userId },
                data: { lastRunAt: new Date(), lastRunStatus },
            })
            .catch(() => null);

        if (updated)
            sendToUser(
                schedule.userId,
                'backupSchedule:update',
                serializeBackupSchedule(updated, schedule.user),
            );

        runs++;
    }
};
