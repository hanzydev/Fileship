export default defineEventHandler(async (event) => {
    superAdminOnly(event);

    const defaults = getSettingDefaults();
    const overrides = await getSettingOverrides();
    const values = { ...defaults, ...overrides };

    return {
        values: { ...values, turnstileSecretKey: '' },
        defaults: { ...defaults, turnstileSecretKey: '' },
        overridden: Object.keys(overrides) as InstanceSettingKey[],
        turnstileSecretKeySet: !!values.turnstileSecretKey,
    };
});
