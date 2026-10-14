import consola from 'consola';
import { defu } from 'defu';
import pg from 'pg';

import { PrismaPg } from '@prisma/adapter-pg';

import { type Prisma, PrismaClient } from '../generated/prisma/client';
import {
    legacyFileshipReport,
    legacyFileshipSchema,
    legacySystemReport,
    legacySystemSchema,
    type StoredReport,
} from '../shared/utils/report';

const UUID_RE = /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/;
const BATCH_SIZE = 1000;

if (!process.env.LEGACY_DATABASE_URL) {
    consola.error('Set LEGACY_DATABASE_URL to the old telemetry database.');
    process.exit(1);
}

const legacy = new pg.Client({ connectionString: process.env.LEGACY_DATABASE_URL });
const prisma = new PrismaClient({
    adapter: new PrismaPg({ connectionString: process.env.DATABASE_URL! }),
});

type Day = { instanceId: string; day: Date; receivedAt: Date; report: StoredReport };

const days = new Map<string, Day>();

const addDay = (instanceId: string, day: Date, receivedAt: Date, report: StoredReport) => {
    const id = instanceId.trim().toLowerCase();
    if (!UUID_RE.test(id)) return;

    const key = `${id}:${day.toISOString()}`;
    const existing = days.get(key);

    days.set(key, {
        instanceId: id,
        day,
        receivedAt: existing && existing.receivedAt > receivedAt ? existing.receivedAt : receivedAt,
        report: defu(report, existing?.report) as StoredReport,
    });
};

const toDay = (day: string) => new Date(`${day}T00:00:00.000Z`);

const latestPerDay = (table: string) =>
    legacy.query(`
        SELECT DISTINCT ON ("projectId", "createdAt"::date)
            *,
            to_char("createdAt", 'YYYY-MM-DD') AS "utcDay",
            "createdAt" AT TIME ZONE 'UTC' AS "receivedAt"
        FROM "${table}"
        WHERE "createdAt" <= now()
        ORDER BY "projectId", "createdAt"::date, "createdAt" DESC
    `);

await legacy.connect();
await legacy.query('SET SESSION CHARACTERISTICS AS TRANSACTION READ ONLY');

const [fileship, system] = await Promise.all([
    latestPerDay('FileshipInfo'),
    latestPerDay('SystemInfo'),
]);

for (const row of fileship.rows) {
    const payload = legacyFileshipSchema.safeParse(row);
    if (payload.success) {
        addDay(
            row.projectId,
            toDay(row.utcDay),
            row.receivedAt,
            legacyFileshipReport(payload.data),
        );
    }
}

for (const row of system.rows) {
    const payload = legacySystemSchema.safeParse({
        cpu: { count: row.cpuCount, model: row.cpuModel },
        memory: { total: row.memoryTotal },
        disk: { total: row.diskTotal, free: row.diskFree },
        os: { platform: row.osPlatform, release: row.osRelease, arch: row.osArch },
        isDocker: row.isDocker,
    });

    if (payload.success) {
        addDay(row.projectId, toDay(row.utcDay), row.receivedAt, legacySystemReport(payload.data));
    }
}

await legacy.end();

const sorted = [...days.values()].sort((a, b) => a.day.getTime() - b.day.getTime());
const instances = new Map<string, Day & { firstSeenAt: Date }>();

for (const day of sorted) {
    const previous = instances.get(day.instanceId);
    day.report = defu(day.report, previous?.report) as StoredReport;

    instances.set(day.instanceId, {
        ...day,
        firstSeenAt: previous?.firstSeenAt ?? day.receivedAt,
    });
}

const existingInstances = new Set(
    (await prisma.instance.findMany({ select: { id: true } })).map((instance) => instance.id),
);

const newInstances = [...instances.values()].filter(
    (instance) => !existingInstances.has(instance.instanceId),
);

for (let i = 0; i < newInstances.length; i += BATCH_SIZE) {
    await prisma.instance.createMany({
        data: newInstances.slice(i, i + BATCH_SIZE).map((instance) => ({
            id: instance.instanceId,
            firstSeenAt: instance.firstSeenAt,
            lastSeenAt: instance.receivedAt,
            version: instance.report.version ?? '',
            users: instance.report.usage?.users?.total ?? 0,
            files: instance.report.usage?.files?.total ?? 0,
            report: instance.report as Prisma.InputJsonValue,
        })),
    });
}

for (const instance of instances.values()) {
    if (!existingInstances.has(instance.instanceId)) continue;

    await prisma.instance.updateMany({
        where: { id: instance.instanceId, firstSeenAt: { gt: instance.firstSeenAt } },
        data: { firstSeenAt: instance.firstSeenAt },
    });
}

let imported = 0;

for (let i = 0; i < sorted.length; i += BATCH_SIZE) {
    const result = await prisma.report.createMany({
        data: sorted.slice(i, i + BATCH_SIZE).map((day) => ({
            instanceId: day.instanceId,
            day: day.day,
            receivedAt: day.receivedAt,
            legacy: true,
            payload: day.report as Prisma.InputJsonValue,
        })),
        skipDuplicates: true,
    });

    imported += result.count;
}

if (sorted.length) {
    await prisma.dailyStat.deleteMany({ where: { day: { gte: sorted[0]!.day } } });
}

await prisma.$disconnect();

consola.success(
    `Imported ${imported} daily reports from ${instances.size} instances (${newInstances.length} new). The panel computes their statistics on its next hourly run or restart, then deletes instances that stopped reporting.`,
);
