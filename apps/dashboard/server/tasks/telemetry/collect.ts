import { randomUUID } from 'node:crypto';
import { existsSync, readFileSync, writeFileSync } from 'node:fs';

import consola from 'consola';
import dayjs from 'dayjs';
import { join } from 'pathe';

import {
    collectRuntime,
    FILE_CATEGORIES,
    type FileCategory,
    reportSchema,
    sendReport,
    type TelemetryUsage,
} from '@fileship/telemetry';

import { AIJobStatus, StorageType, UserPermission } from '#shared/prisma/enums';

import pkg from '../../../../../package.json';

const getInstanceId = () => {
    const path = join(dataDirectory, '.fileship_project_id');
    const id = existsSync(path) ? readFileSync(path, 'utf-8').trim() : '';
    if (id) return id;

    const newId = randomUUID();
    writeFileSync(path, newId);

    return newId;
};

const collectUsage = async (): Promise<TelemetryUsage> => {
    const active = { deletedAt: null };
    const trashed = { deletedAt: { not: null } };
    const since = dayjs().subtract(30, 'day').toDate();

    const [
        users,
        admins,
        totp,
        passkeys,
        uploadTokens,
        files,
        trashedFiles,
        folders,
        publicFolders,
        trashedFolders,
        notes,
        publicNotes,
        trashedNotes,
        links,
        protectedLinks,
        backups,
        inboxes,
        enabledInboxes,
        views,
        storages,
    ] = await prisma.$transaction([
        prisma.user.count(),
        prisma.user.count({
            where: { OR: [{ superAdmin: true }, { permissions: { has: UserPermission.Admin } }] },
        }),
        prisma.user.count({ where: { totpEnabled: true } }),
        prisma.user.count({ where: { credentials: { some: {} } } }),
        prisma.user.count({ where: { uploadTokenHash: { not: null } } }),
        prisma.file.aggregate({ where: active, _count: true, _sum: { size: true } }),
        prisma.file.count({ where: trashed }),
        prisma.folder.count({ where: active }),
        prisma.folder.count({ where: { ...active, public: true } }),
        prisma.folder.count({ where: trashed }),
        prisma.note.count({ where: active }),
        prisma.note.count({ where: { ...active, public: true } }),
        prisma.note.count({ where: trashed }),
        prisma.link.aggregate({ _count: true, _sum: { clicks: true } }),
        prisma.link.count({ where: { password: { not: null } } }),
        prisma.backup.aggregate({ _count: true, _sum: { size: true } }),
        prisma.inbox.count(),
        prisma.inbox.count({ where: { enabled: true } }),
        prisma.view.count({ where: { createdAt: { gte: since } } }),
        prisma.storage.groupBy({ by: ['type'], _count: true, orderBy: { type: 'asc' } }),
    ]);

    const categories = await prisma.$transaction(
        FILE_CATEGORIES.map((category) =>
            prisma.file.aggregate({
                where: { ...active, ...fileCategoryWhere(category) },
                _count: true,
                _sum: { size: true },
            }),
        ),
    );

    const activeStorageId = await getActiveStorageId();
    const activeStorage = activeStorageId
        ? await prisma.storage.findUnique({
              where: { id: activeStorageId },
              select: { type: true },
          })
        : null;

    return {
        users: { total: users, admins, totp, passkeys, uploadTokens },
        files: {
            total: files._count,
            trashed: trashedFiles,
            bytes: Number(files._sum.size ?? 0),
            categories: Object.fromEntries(
                FILE_CATEGORIES.map((category, index) => [
                    category,
                    {
                        total: categories[index]!._count,
                        bytes: Number(categories[index]!._sum.size ?? 0),
                    },
                ]),
            ) as Record<FileCategory, { total: number; bytes: number }>,
        },
        folders: { total: folders, public: publicFolders, trashed: trashedFolders },
        notes: { total: notes, public: publicNotes, trashed: trashedNotes },
        links: {
            total: links._count,
            clicks: links._sum.clicks ?? 0,
            protected: protectedLinks,
        },
        backups: { total: backups._count, bytes: Number(backups._sum.size ?? 0) },
        inboxes: { total: inboxes, enabled: enabledInboxes },
        views: { last30Days: views },
        storages: {
            active: activeStorage?.type ?? 'local',
            types: Object.fromEntries(
                Object.values(StorageType).map((type) => [
                    type,
                    storages.find((row) => row.type === type)?._count ?? 0,
                ]),
            ) as Record<StorageType, number>,
        },
    };
};

const collectFeatures = async () => {
    const settings = await getSettings();

    const [analyzedFiles, sensitiveFiles, jobs] = await prisma.$transaction([
        prisma.file.count({ where: { deletedAt: null, aiAnalyzedAt: { not: null } } }),
        prisma.file.count({ where: { deletedAt: null, piiDetected: true } }),
        prisma.aIJob.groupBy({ by: ['status'], _count: true, orderBy: { status: 'asc' } }),
    ]);

    const jobCount = (...statuses: AIJobStatus[]) =>
        jobs
            .filter((row) => statuses.includes(row.status))
            .reduce((sum, row) => sum + (row._count as number), 0);

    return {
        registration: settings.registrationEnabled,
        trashRetentionDays: settings.trashRetentionDays,
        logLevel: settings.logLevel,
        turnstile: !!settings.turnstileSiteKey && !!settings.turnstileSecretKey,
        removeExifData: settings.removeExifData,
        ai: {
            enabled: settings.aiEnabled,
            preset: settings.aiPreset,
            analyzedFiles,
            sensitiveFiles,
            pendingJobs: jobCount(AIJobStatus.Pending, AIJobStatus.Processing),
            failedJobs: jobCount(AIJobStatus.Failed),
        },
    };
};

export default defineTask({
    meta: {
        name: 'telemetry:collect',
        description: 'Sends an anonymous usage report.',
    },
    async run() {
        const { telemetryUrl } = useRuntimeConfig();
        const { telemetryEnabled } = await getSettings();

        if (!telemetryEnabled || (import.meta.dev && !telemetryUrl)) {
            return { result: 'disabled' };
        }

        try {
            const [runtime, usage, features] = await Promise.all([
                collectRuntime(dataDirectory),
                collectUsage(),
                collectFeatures(),
            ]);

            const report = reportSchema.parse({
                id: getInstanceId(),
                version: pkg.version,
                sentAt: Date.now(),
                runtime,
                usage,
                features,
            });

            await sendReport(report, telemetryUrl || undefined);

            return { result: 'success' };
        } catch (error: any) {
            consola.warn(
                `${dayjs().format('YYYY-MM-DD HH:mm:ss')} - Failed to send telemetry: ${error.message}`,
            );

            return { result: 'error' };
        }
    },
});
