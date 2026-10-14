import { defu } from 'defu';
import type { H3Event } from 'h3';

import type { Prisma } from '~~/generated/prisma/client';

const MAX_CLOCK_SKEW = 24 * 60 * 60 * 1000;

export const limitReports = async (event: H3Event, instanceId: string) => {
    await rateLimit(`report:${instanceId}`, 30, 60 * 60_000);
    await rateLimit(
        `reportSender:${getRequestIP(event, { xForwardedFor: true })}`,
        120,
        60 * 60_000,
    );
};

export const saveReport = async (
    instanceId: string,
    report: StoredReport,
    options: { sentAt?: number; legacy?: boolean } = {},
) => {
    const receivedAt = new Date();
    const day = startOfDay(receivedAt);
    const sentAt =
        options.sentAt && Math.abs(options.sentAt - receivedAt.getTime()) < MAX_CLOCK_SKEW
            ? new Date(options.sentAt)
            : null;

    for (let attempt = 0; ; attempt++) {
        try {
            return await prisma.$transaction(async (tx) => {
                const existing = options.legacy
                    ? await tx.report.findUnique({
                          where: { instanceId_day: { instanceId, day } },
                          select: { payload: true },
                      })
                    : null;

                const instance = options.legacy
                    ? await tx.instance.findUnique({
                          where: { id: instanceId },
                          select: { report: true },
                      })
                    : null;

                const payload = defu(
                    report,
                    existing?.payload as StoredReport,
                    instance?.report as StoredReport,
                ) as StoredReport;

                const latest = {
                    lastSeenAt: receivedAt,
                    ...(payload.version && { version: payload.version }),
                    users: payload.usage?.users?.total ?? 0,
                    files: payload.usage?.files?.total ?? 0,
                    storage: BigInt(Math.round(payload.usage?.files?.bytes ?? 0)),
                    report: payload as Prisma.InputJsonValue,
                };

                await tx.instance.upsert({
                    where: { id: instanceId },
                    create: { id: instanceId, firstSeenAt: receivedAt, ...latest },
                    update: latest,
                });

                await tx.report.upsert({
                    where: { instanceId_day: { instanceId, day } },
                    create: {
                        instanceId,
                        day,
                        receivedAt,
                        sentAt,
                        legacy: !!options.legacy,
                        payload: payload as Prisma.InputJsonValue,
                    },
                    update: {
                        receivedAt,
                        sentAt,
                        legacy: !!options.legacy,
                        payload: payload as Prisma.InputJsonValue,
                    },
                });
            });
        } catch (error: any) {
            if (error?.code !== 'P2002' || attempt >= 2) throw error;
        }
    }
};
