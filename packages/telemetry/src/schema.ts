import { z } from 'zod';

const count = z.number().int().min(0);
const bytes = z.number().min(0);
const text = (max: number) => z.string().trim().max(max);

export const FILE_CATEGORIES = [
    'image',
    'video',
    'audio',
    'document',
    'archive',
    'code',
    'other',
] as const;

export const STORAGE_TYPES = ['local', 'S3', 'SFTP', 'FTP'] as const;

export const AI_PRESETS = ['low', 'medium', 'high'] as const;

export const runtimeSchema = z.object({
    node: text(32),
    platform: text(32),
    release: text(128),
    arch: text(32),
    cpuCount: count.max(4096),
    cpuModel: text(128),
    memoryTotal: bytes,
    diskTotal: bytes,
    diskFree: bytes,
    docker: z.boolean(),
    uptime: count,
});

export const usageSchema = z.object({
    users: z.object({
        total: count,
        admins: count,
        totp: count,
        passkeys: count,
        uploadTokens: count,
    }),
    files: z.object({
        total: count,
        trashed: count,
        bytes,
        categories: z.record(z.enum(FILE_CATEGORIES), z.object({ total: count, bytes })),
    }),
    folders: z.object({ total: count, public: count, trashed: count }),
    notes: z.object({ total: count, public: count, trashed: count }),
    links: z.object({ total: count, clicks: count, protected: count }),
    backups: z.object({ total: count, bytes }),
    inboxes: z.object({ total: count, enabled: count }),
    views: z.object({ last30Days: count }),
    storages: z.object({
        active: z.enum(STORAGE_TYPES),
        types: z.record(z.enum(STORAGE_TYPES).exclude(['local']), count),
    }),
});

export const featuresSchema = z.object({
    registration: z.boolean(),
    trashRetentionDays: count.max(365),
    logLevel: count.max(2),
    turnstile: z.boolean(),
    removeExifData: z.boolean(),
    ai: z.object({
        enabled: z.boolean(),
        preset: z.enum(AI_PRESETS),
        analyzedFiles: count,
        sensitiveFiles: count,
        pendingJobs: count,
        failedJobs: count,
    }),
});

export const reportSchema = z.object({
    id: z.guid(),
    version: text(32),
    sentAt: z.number().int().min(0),
    runtime: runtimeSchema,
    usage: usageSchema,
    features: featuresSchema,
});

export type TelemetryRuntime = z.infer<typeof runtimeSchema>;
export type TelemetryUsage = z.infer<typeof usageSchema>;
export type TelemetryFeatures = z.infer<typeof featuresSchema>;
export type TelemetryReport = z.infer<typeof reportSchema>;
export type FileCategory = (typeof FILE_CATEGORIES)[number];
export type TelemetryStorageType = (typeof STORAGE_TYPES)[number];
