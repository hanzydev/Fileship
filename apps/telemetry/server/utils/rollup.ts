import { FILE_CATEGORIES } from '@fileship/telemetry';

import type { Prisma } from '~~/generated/prisma/client';

const MEMORY_SIZES = [1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024];
const DISK_SIZES = [16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384];
const GB = 1024 ** 3;

const nearest = (value: number, sizes: number[]) =>
    sizes.reduce((best, size) =>
        Math.abs(Math.log2(size) - Math.log2(value)) < Math.abs(Math.log2(best) - Math.log2(value))
            ? size
            : best,
    );

const formatSize = (gigabytes: number) =>
    gigabytes >= 1024 ? `${gigabytes / 1024} TB` : `${gigabytes} GB`;

const sizeBucket = (bytes: number | undefined, sizes: number[]) =>
    bytes ? formatSize(nearest(bytes / GB, sizes)) : null;

const retentionBucket = (days: number) =>
    days === 0 ? 'Off' : days <= 7 ? '1–7 days' : days <= 30 ? '8–30 days' : '31+ days';

const top = (counts: Record<string, number>, limit: number) =>
    Object.fromEntries(
        Object.entries(counts)
            .sort((a, b) => b[1] - a[1])
            .slice(0, limit),
    );

export const summarize = (reports: StoredReport[]) => {
    const tally = (counts: Record<string, number>, key: string | null | undefined) => {
        if (key) counts[key] = (counts[key] ?? 0) + 1;
    };

    const versions: Record<string, number> = {};
    const platforms: Record<string, number> = {};
    const arches: Record<string, number> = {};
    const node: Record<string, number> = {};
    const cpuCores: Record<string, number> = {};
    const cpuModels: Record<string, number> = {};
    const memory: Record<string, number> = {};
    const disk: Record<string, number> = {};
    const presets: Record<string, number> = {};
    const activeStorages: Record<string, number> = {};
    const storageSetups: Record<string, number> = {};
    const trashRetention: Record<string, number> = {};
    const logLevels: Record<string, number> = {};
    const categories = Object.fromEntries(
        FILE_CATEGORIES.map((category) => [category, { total: 0, bytes: 0 }]),
    );

    const totals = {
        users: 0,
        files: 0,
        storage: 0,
        links: 0,
        folders: 0,
        notes: 0,
        clicks: 0,
        views: 0,
        backups: 0,
        backupBytes: 0,
        trashedFiles: 0,
        admins: 0,
        totp: 0,
        passkeys: 0,
        uploadTokens: 0,
        inboxes: 0,
        analyzedFiles: 0,
        sensitiveFiles: 0,
    };

    const flags = {
        full: 0,
        runtime: 0,
        docker: 0,
        aiEnabled: 0,
        registration: 0,
        turnstile: 0,
        removeExifData: 0,
    };

    for (const { version, runtime, usage, features } of reports) {
        tally(versions, version || 'Unknown');

        totals.users += usage?.users?.total ?? 0;
        totals.files += usage?.files?.total ?? 0;
        totals.folders += usage?.folders?.total ?? 0;
        totals.notes += usage?.notes?.total ?? 0;

        if (runtime?.platform) {
            flags.runtime++;
            if (runtime.docker) flags.docker++;

            const nodeMajor = runtime.node?.match(/^v?(\d+)/)?.[1];
            const cores = runtime.cpuCount ?? 0;

            tally(platforms, runtime.platform);
            tally(arches, runtime.arch);
            tally(node, nodeMajor && `Node ${nodeMajor}`);
            tally(cpuCores, cores ? (cores >= 32 ? '32+' : `${cores}`) : null);
            tally(cpuModels, runtime.cpuModel);
            tally(memory, sizeBucket(runtime.memoryTotal, MEMORY_SIZES));
            tally(disk, sizeBucket(runtime.diskTotal, DISK_SIZES));
        }

        if (!usage?.links || !features) continue;

        flags.full++;

        totals.storage += usage.files?.bytes ?? 0;
        totals.links += usage.links.total ?? 0;
        totals.clicks += usage.links.clicks ?? 0;
        totals.views += usage.views?.last30Days ?? 0;
        totals.backups += usage.backups?.total ?? 0;
        totals.backupBytes += usage.backups?.bytes ?? 0;
        totals.trashedFiles += usage.files?.trashed ?? 0;
        totals.admins += usage.users?.admins ?? 0;
        totals.totp += usage.users?.totp ?? 0;
        totals.passkeys += usage.users?.passkeys ?? 0;
        totals.uploadTokens += usage.users?.uploadTokens ?? 0;
        totals.inboxes += usage.inboxes?.total ?? 0;
        totals.analyzedFiles += features.ai?.analyzedFiles ?? 0;
        totals.sensitiveFiles += features.ai?.sensitiveFiles ?? 0;

        for (const category of FILE_CATEGORIES) {
            categories[category]!.total += usage.files?.categories?.[category]?.total ?? 0;
            categories[category]!.bytes += usage.files?.categories?.[category]?.bytes ?? 0;
        }

        tally(activeStorages, usage.storages?.active);

        const storageTypes = Object.entries(usage.storages?.types ?? {})
            .filter(([, amount]) => amount)
            .map(([type]) => type)
            .sort();

        tally(
            storageSetups,
            storageTypes.length ? storageTypes.join(' + ') : 'Data directory only',
        );

        if (features.ai?.enabled) {
            flags.aiEnabled++;
            tally(presets, features.ai.preset);
        }

        if (features.registration) flags.registration++;
        if (features.turnstile) flags.turnstile++;
        if (features.removeExifData) flags.removeExifData++;

        tally(trashRetention, retentionBucket(features.trashRetentionDays ?? 0));
        tally(logLevels, `${features.logLevel ?? 2}`);
    }

    return {
        instances: reports.length,
        users: totals.users,
        files: totals.files,
        storage: BigInt(Math.round(totals.storage)),
        links: totals.links,
        versions,
        data: {
            ...totals,
            ...flags,
            categories,
            platforms,
            arches,
            node,
            cpuCores,
            cpuModels: top(cpuModels, 10),
            memory,
            disk,
            presets,
            activeStorages,
            storageSetups,
            trashRetention,
            logLevels,
        },
    };
};

export type DayData = ReturnType<typeof summarize>['data'];

export const computeDay = async (day: Date) => {
    const reports = await prisma.$queryRaw<{ payload: StoredReport }[]>`
        SELECT DISTINCT ON ("instanceId") "payload"
        FROM "Report"
        WHERE "day" BETWEEN ${toDateString(addDays(day, 1 - ACTIVE_DAYS))}::date
            AND ${toDateString(day)}::date
        ORDER BY "instanceId", "day" DESC
    `;

    const { data, versions, ...summary } = summarize(reports.map((report) => report.payload));

    const stat = {
        ...summary,
        versions: versions as Prisma.InputJsonValue,
        data: data as Prisma.InputJsonValue,
    };

    await prisma.dailyStat.upsert({
        where: { day },
        create: { day, ...stat },
        update: stat,
    });
};

export const rollup = async () => {
    const [first, computed] = await Promise.all([
        prisma.report.findFirst({ orderBy: { day: 'asc' }, select: { day: true } }),
        prisma.dailyStat.findMany({ select: { day: true } }),
    ]);

    if (!first) return 0;

    const today = startOfDay(new Date());
    const known = new Set(computed.map((stat) => stat.day.getTime()));
    const days: Date[] = [];

    for (let day = first.day; day <= today; day = addDays(day, 1)) {
        if (!known.has(day.getTime()) || day >= addDays(today, -1)) days.push(day);
    }

    for (const day of days) await computeDay(day);

    return days.length;
};
