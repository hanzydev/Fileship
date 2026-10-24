const hideSecrets = (settings: InstanceSettings) =>
    ({
        ...settings,
        ...Object.fromEntries(secretSettingKeys.map((key) => [key, ''])),
    }) as InstanceSettings;

export default defineEventHandler(async (event) => {
    superAdminOnly(event);

    const defaults = getSettingDefaults();
    const overrides = await getSettingOverrides();
    const values = { ...defaults, ...overrides };

    return {
        values: hideSecrets(values),
        defaults: hideSecrets(defaults),
        overridden: Object.keys(overrides) as InstanceSettingKey[],
        secretsSet: Object.fromEntries(
            secretSettingKeys.map((key) => [key, !!values[key]]),
        ) as Record<SecretSettingKey, boolean>,
    };
});
