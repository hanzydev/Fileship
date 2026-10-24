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
        scheduledBackupsEnabled: parseDefault(
            'scheduledBackupsEnabled',
            !!config.scheduledBackupsEnabled,
            true,
        ),
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
            [UserPermission.UploadFiles, UserPermission.TakeNotes, UserPermission.ShortenLinks],
        ),
        registrationUsableSpace: parseDefault(
            'registrationUsableSpace',
            +config.registration.usableSpace,
            -1,
        ),
        turnstileSiteKey: parseDefault('turnstileSiteKey', config.public.turnstile.siteKey, ''),
        turnstileSecretKey: parseDefault('turnstileSecretKey', config.turnstile.secretKey, ''),
        aiEnabled: parseDefault('aiEnabled', !!config.public.aiEnabled, true),
        aiPreset: parseDefault('aiPreset', config.ai.preset, 'medium'),
        aiQueueConcurrency: parseDefault('aiQueueConcurrency', +config.ai.queueConcurrency, 1),
        aiInferenceThreads: parseDefault('aiInferenceThreads', +config.ai.inferenceThreads, 2),
        aiJobLeaseMs: parseDefault('aiJobLeaseMs', +config.ai.jobLeaseMs, 2 * 60_000),
        logLevel: parseDefault('logLevel', +config.logLevel, 2),
        telemetryEnabled: parseDefault('telemetryEnabled', !!config.telemetryEnabled, true),
        siteUrl: parseDefault('siteUrl', config.siteUrl, ''),
        smtpHost: parseDefault('smtpHost', config.smtp.host, ''),
        smtpPort: parseDefault('smtpPort', +config.smtp.port, 587),
        smtpSecurity: parseDefault('smtpSecurity', config.smtp.security, 'starttls'),
        smtpUser: parseDefault('smtpUser', config.smtp.user, ''),
        smtpPassword: parseDefault('smtpPassword', config.smtp.password, ''),
        smtpFrom: parseDefault('smtpFrom', config.smtp.from, ''),
        activeStorageId: null,
    };
};

const encryptedSettingKeys: InstanceSettingKey[] = ['smtpPassword'];

let cache: { overrides: Partial<InstanceSettings>; expiresAt: number } | null = null;
let pending: { version: number; overrides: Promise<Partial<InstanceSettings>> } | null = null;
let version = 0;

const loadOverrides = async () => {
    const rows = await prisma.setting.findMany();
    const overrides: Partial<InstanceSettings> = {};

    for (const row of rows) {
        const field = settingsSchema.shape[row.key as InstanceSettingKey];
        const value = encryptedSettingKeys.includes(row.key as InstanceSettingKey)
            ? await decrypt(row.value as string).catch(() => null)
            : row.value;
        const result = field?.safeParse(value);
        if (result?.success) (overrides as any)[row.key] = result.data;
    }

    return overrides;
};

export const getSettingOverrides = async () => {
    if (cache && cache.expiresAt > Date.now()) return cache.overrides;

    let load = pending;

    if (load?.version !== version) {
        const current = { version, overrides: loadOverrides() };

        pending = load = current;
        current.overrides
            .finally(() => {
                if (pending === current) pending = null;
            })
            .catch(() => null);
    }

    const overrides = await load.overrides;

    if (load.version === version) cache = { overrides, expiresAt: Date.now() + 10_000 };
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
    const storage = await getStorage(settings.activeStorageId).catch(() => null);

    return {
        ...(Object.fromEntries(publicSettingKeys.map((key) => [key, settings[key]])) as Omit<
            PublicSettings,
            'uploadConcurrency' | 'emailEnabled'
        >),
        uploadConcurrency: storage && !storage.isSequential ? 4 : 1,
        emailEnabled: isEmailConfigured(settings),
    };
};

export const updateSettings = async (patch: {
    [K in InstanceSettingKey]?: InstanceSettings[K] | null;
}) => {
    const entries = await Promise.all(
        (Object.entries(patch) as [InstanceSettingKey, unknown][]).map(
            async ([key, value]) =>
                [
                    key,
                    value !== null && encryptedSettingKeys.includes(key)
                        ? await encrypt(value as string)
                        : value,
                ] as const,
        ),
    );

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

    version++;
    cache = null;
};

export const isAiEnabled = async () => (await getSettings()).aiEnabled;
