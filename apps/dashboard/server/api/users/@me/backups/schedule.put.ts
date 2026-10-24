import { z } from 'zod';

import { BackupFrequency } from '#shared/prisma/enums';

const validationSchema = z
    .object({
        frequency: z.enum(BackupFrequency),
        hour: z.number().int().min(0).max(23),
        weekday: z.number().int().min(0).max(6),
        timezone: z.string().max(64).refine(isValidTimezone),
        keep: z.number().int().min(1).max(100),
    })
    .strict();

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;

    const body = await readValidatedBody(event, validationSchema.safeParse);
    if (!body.success) {
        throw invalidBodyError(body.error);
    }

    const limit = backupLimitOf(currentUser);

    if (body.data.keep > limit) {
        throw apiError(400, 'errorsUsers.backupKeepAboveLimit', { limit });
    }

    const data = { ...body.data, nextRunAt: nextBackupRun(body.data) };

    const schedule = serializeBackupSchedule(
        await prisma.backupSchedule.upsert({
            where: { userId: currentUser.id },
            create: { userId: currentUser.id, ...data },
            update: data,
        }),
        currentUser,
    );

    sendToUser(currentUser.id, 'backupSchedule:update', schedule);

    return schedule;
});
