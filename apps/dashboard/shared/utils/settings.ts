import { z } from 'zod';

import { UserPermission } from '#shared/prisma/enums';

export const settingsSchema = z.object({
    siteName: z.string().trim().min(1, 'Site name is required').max(64),
    returnHttps: z.enum(['auto', 'true', 'false']),
    fileChunkSize: z.number().int().min(5).max(100),
    removeExifData: z.boolean(),
    avatarMaxSize: z.number().min(1).max(100),
    registrationEnabled: z.boolean(),
    registrationPermissions: z.array(
        z.enum([UserPermission.UploadFiles, UserPermission.TakeNotes]),
    ),
    registrationUsableSpace: z.number().int().min(-1),
    turnstileSiteKey: z.string().trim().max(256),
    turnstileSecretKey: z.string().trim().max(256),
    aiEnabled: z.boolean(),
    aiQueueConcurrency: z.number().int().min(1).max(8),
    aiInferenceThreads: z.number().int().min(1).max(64),
    aiVideoMaxFrames: z.number().int().min(1).max(120),
    aiVideoSampleIntervalSeconds: z.number().min(1).max(3600),
    aiJobLeaseMs: z.number().int().min(60_000),
    aiJobDequeueRetries: z.number().int().min(1).max(50),
    logLevel: z.union([z.literal(0), z.literal(1), z.literal(2)]),
    telemetryEnabled: z.boolean(),
    activeStorageId: z.string().nullable(),
});

export type InstanceSettings = z.infer<typeof settingsSchema>;
export type InstanceSettingKey = keyof InstanceSettings;

export const publicSettingKeys = [
    'siteName',
    'returnHttps',
    'fileChunkSize',
    'registrationEnabled',
    'turnstileSiteKey',
    'aiEnabled',
] as const satisfies InstanceSettingKey[];

export type PublicSettings = Pick<InstanceSettings, (typeof publicSettingKeys)[number]>;
