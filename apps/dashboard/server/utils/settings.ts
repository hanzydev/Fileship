import { UserPermission } from '#shared/prisma/enums';

const parseDefault = <K extends InstanceSettingKey>(
    key: K,
    value: unknown,
    fallback: InstanceSettings[K],
) => {
    const result = settingsSchema.shape[key].safeParse(value);
    return (result.success ? result.data : fallback) as InstanceSettings[K];
};

export const getSettingDefaults = (): InstanceSettings => {
    const config = useRuntimeConfig();

    return {
        siteName: parseDefault('siteName', config.public.site.name, 'Fileship'),
        returnHttps: parseDefault('returnHttps', String(config.public.returnHttps), 'auto'),
        fileChunkSize: parseDefault('fileChunkSize', +config.public.fileChunkSize, 25),
        removeExifData: parseDefault('removeExifData', !!config.removeExifData, true),
        avatarMaxSize: parseDefault('avatarMaxSize', +config.avatarMaxSize, 4),
        trashRetentionDays: parseDefault('trashRetentionDays', +config.trashRetentionDays, 30),
        registrationEnabled: parseDefault(
            'registrationEnabled',
            !!config.public.registrationEnabled,
            false,
        ),
        registrationPermissions: parseDefault(
            'registrationPermissions',
            String(config.registration.permissions)
                .split(',')
                .map((permission) => permission.trim())
                .filter((permission) =>
                    (
                        [
                            UserPermission.UploadFiles,
                            UserPermission.TakeNotes,
                            UserPermission.ShortenLinks,
                        ] as string[]
                    ).includes(permission),
                ),
            [UserPermission.UploadFiles, UserPermission.TakeNotes],
        ),
        registrationUsableSpace: parseDefault(
            'registrationUsableSpace',
            +config.registration.usableSpace,
            -1,
        ),
        turnstileSiteKey: parseDefault('turnstileSiteKey', config.public.turnstile.siteKey, ''),
        turnstileSecretKey: parseDefault('turnstileSecretKey', config.turnstile.secretKey, ''),
        aiEnabled: parseDefault('aiEnabled', !!config.public.aiEnabled, true),
        aiQueueConcurrency: parseDefault('aiQueueConcurrency', +config.ai.queueConcurrency, 1),
        aiInferenceThreads: parseDefault('aiInferenceThreads', +config.ai.inferenceThreads, 2),
        aiVideoMaxFrames: parseDefault('aiVideoMaxFrames', +config.ai.videoMaxFrames, 24),
        aiVideoSampleIntervalSeconds: parseDefault(
            'aiVideoSampleIntervalSeconds',
            +config.ai.videoSampleIntervalSeconds,
            10,
        ),
        aiJobLeaseMs: parseDefault('aiJobLeaseMs', +config.ai.jobLeaseMs, 15 * 60_000),
        aiJobDequeueRetries: parseDefault('aiJobDequeueRetries', +config.ai.jobDequeueRetries, 5),
        logLevel: parseDefault('logLevel', +config.logLevel, 2),
        telemetryEnabled: parseDefault('telemetryEnabled', !!config.telemetryEnabled, true),
        activeStorageId: null,
    };
};

let cache: { overrides: Partial<InstanceSettings>; expiresAt: number } | null = null;
let pending: Promise<Partial<InstanceSettings>> | null = null;

const loadOverrides = async () => {
    const rows = await prisma.setting.findMany();
    const overrides: Partial<InstanceSettings> = {};

    for (const row of rows) {
        const field = settingsSchema.shape[row.key as InstanceSettingKey];
        const result = field?.safeParse(row.value);
        if (result?.success) (overrides as any)[row.key] = result.data;
    }

    return overrides;
};

export const getSettingOverrides = async () => {
    if (cache && cache.expiresAt > Date.now()) return cache.overrides;

    pending ??= loadOverrides().finally(() => (pending = null));
    const overrides = await pending;

    cache = { overrides, expiresAt: Date.now() + 10_000 };
    return overrides;
};

export const getSettings = async (): Promise<InstanceSettings> => ({
    ...getSettingDefaults(),
    ...(await getSettingOverrides()),
});

export const getSettingsSync = (): InstanceSettings => {
    if (!cache || cache.expiresAt <= Date.now()) getSettingOverrides().catch(() => null);

    return { ...getSettingDefaults(), ...cache?.overrides };
};

export const getPublicSettings = async (): Promise<PublicSettings> => {
    const settings = await getSettings();
    return Object.fromEntries(
        publicSettingKeys.map((key) => [key, settings[key]]),
    ) as PublicSettings;
};

export const updateSettings = async (patch: {
    [K in InstanceSettingKey]?: InstanceSettings[K] | null;
}) => {
    const entries = Object.entries(patch) as [InstanceSettingKey, unknown][];

    await prisma.$transaction(
        entries.map(([key, value]) =>
            value === null
                ? prisma.setting.deleteMany({ where: { key } })
                : prisma.setting.upsert({
                      where: { key },
                      create: { key, value: value as any },
                      update: { value: value as any },
                  }),
        ),
    );

    cache = null;
};

export const isAiEnabled = async () => (await getSettings()).aiEnabled;
