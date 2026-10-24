import { z } from 'zod';

import { UserPermission } from '#shared/prisma/enums';

export const settingsSchema = z.object({
    siteName: z.string().trim().min(1, 'instance.siteNameRequired').max(64),
    returnHttps: z.enum(['auto', 'true', 'false']),
    fileChunkSize: z.number().int().min(5).max(100),
    removeExifData: z.boolean(),
    avatarMaxSize: z.number().min(1).max(100),
    trashRetentionDays: z.number().int().min(0).max(365),
    scheduledBackupsEnabled: z.boolean(),
    registrationEnabled: z.boolean(),
    registrationPermissions: z.array(
        z.enum([UserPermission.UploadFiles, UserPermission.TakeNotes, UserPermission.ShortenLinks]),
    ),
    registrationUsableSpace: z.number().int().min(-1),
    turnstileSiteKey: z.string().trim().max(256),
    turnstileSecretKey: z.string().trim().max(256),
    aiEnabled: z.boolean(),
    aiPreset: z.enum(['low', 'medium', 'high']),
    aiQueueConcurrency: z.number().int().min(1).max(8),
    aiInferenceThreads: z.number().int().min(1).max(64),
    aiJobLeaseMs: z.number().int().min(60_000),
    logLevel: z.union([z.literal(0), z.literal(1), z.literal(2)]),
    telemetryEnabled: z.boolean(),
    siteUrl: z
        .string()
        .trim()
        .max(256)
        .regex(/^(https?:\/\/[^\s/?#]+\/?)?$/i, 'instance.siteUrlInvalid'),
    smtpHost: z.string().trim().max(256),
    smtpPort: z.number().int().min(1).max(65535),
    smtpSecurity: z.enum(['none', 'starttls', 'tls']),
    smtpUser: z.string().trim().max(256),
    smtpPassword: z.string().max(256),
    smtpFrom: z.union([z.literal(''), z.email().max(256)]),
    activeStorageId: z.string().nullable(),
});

export type InstanceSettings = z.infer<typeof settingsSchema>;
export type InstanceSettingKey = keyof InstanceSettings;

export const secretSettingKeys = [
    'turnstileSecretKey',
    'smtpPassword',
] as const satisfies InstanceSettingKey[];

export type SecretSettingKey = (typeof secretSettingKeys)[number];

export const publicSettingKeys = [
    'siteName',
    'returnHttps',
    'fileChunkSize',
    'trashRetentionDays',
    'scheduledBackupsEnabled',
    'registrationEnabled',
    'turnstileSiteKey',
    'aiEnabled',
] as const satisfies InstanceSettingKey[];

export type PublicSettings = Pick<InstanceSettings, (typeof publicSettingKeys)[number]> & {
    uploadConcurrency: number;
    emailEnabled: boolean;
};
