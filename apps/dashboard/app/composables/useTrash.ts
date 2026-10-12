export type TrashSelection = { files?: string[]; folders?: string[]; notes?: string[] };

export const useTrash = () => {
    const publicSettings = usePublicSettings();
    const { $toast } = useNuxtApp();

    const retentionDays = computed(() => publicSettings.value.trashRetentionDays);
    const isEnabled = computed(() => retentionDays.value > 0);

    const deleteConfirmation = (label: string) =>
        isEnabled.value
            ? {
                  title: `Move ${label} to trash?`,
                  description: `You can restore it from the trash for ${pluralize(retentionDays.value, 'day')}.`,
                  confirmText: 'Move to trash',
                  destructive: true,
              }
            : {
                  title: `Delete ${label}?`,
                  description: 'This action cannot be undone.',
                  confirmText: 'Delete',
                  destructive: true,
              };

    const restore = async (selection: TrashSelection) => {
        try {
            await $fetch('/api/trash/restore', { method: 'POST', body: selection });
            $toast.success('Restored');

            return true;
        } catch (error: any) {
            $toast.error(getErrorMessage(error));
            return false;
        }
    };

    const notifyDeleted = (label: string, selection: TrashSelection) => {
        if (!isEnabled.value) return $toast.success(`${label} deleted`);

        $toast.success(`${label} moved to trash`, {
            action: { label: 'Undo', onClick: () => restore(selection) },
        });
    };

    return { retentionDays, isEnabled, deleteConfirmation, restore, notifyDeleted };
};
