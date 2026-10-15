import type { Settings } from '../../shared/ipc';

type Section =
    'capture' | 'afterCapture' | 'afterUpload' | 'upload' | 'save' | 'recording' | 'audio' | 'gif';

export const useSettings = () => useState<Settings | null>('settings', () => null);

export const updateSettings = async (patch: Partial<Settings>) => {
    const settings = useSettings();

    try {
        settings.value = await desktop('settings:set', patch);
    } catch (error: any) {
        useNuxtApp().$toast.error(error.message);
    }
};

export const updateSection = <K extends Section>(section: K, patch: Partial<Settings[K]>) => {
    const settings = useSettings();
    if (!settings.value) return;

    return updateSettings({ [section]: { ...settings.value[section], ...patch } });
};
