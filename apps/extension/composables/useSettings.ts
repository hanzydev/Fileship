export const useSettings = () => {
    const settings = ref<Settings | null>(null);

    settingsItem.getValue().then((value) => (settings.value = value));
    onScopeDispose(settingsItem.watch((value) => (settings.value = value)));

    return settings;
};
