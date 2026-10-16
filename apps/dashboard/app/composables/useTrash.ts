export type TrashSelection = { files?: string[]; folders?: string[]; notes?: string[] };

export const useTrash = () => {
    const publicSettings = usePublicSettings();
    const { $toast, $i18n } = useNuxtApp();
    const { t } = $i18n;

    const retentionDays = computed(() => publicSettings.value.trashRetentionDays);
    const isEnabled = computed(() => retentionDays.value > 0);

    const deleteConfirmation = (label: string) =>
        isEnabled.value
            ? {
                  title: t('trash.moveTitle', { label }),
                  description: t('trash.moveDescription', retentionDays.value),
                  confirmText: t('trash.moveToTrash'),
                  destructive: true,
              }
            : {
                  title: t('trash.deleteTitle', { label }),
                  description: t('trash.cannotUndo'),
                  confirmText: t('trash.delete'),
                  destructive: true,
              };

    const restore = async (selection: TrashSelection) => {
        const id = $toast.loading(t('trash.restoring'));

        try {
            await $fetch('/api/trash/restore', { method: 'POST', body: selection });
            $toast.success(t('trash.restored'), { id });

            return true;
        } catch (error: any) {
            $toast.error(getErrorMessage(error), { id });
            return false;
        }
    };

    const notifyDeleted = (label: string, selection: TrashSelection) => {
        if (!isEnabled.value) return $toast.success(t('trash.deleted', { label }));

        $toast.success(t('trash.movedToTrash', { label }), {
            action: { label: t('trash.undo'), onClick: () => restore(selection) },
        });
    };

    return { retentionDays, isEnabled, deleteConfirmation, restore, notifyDeleted };
};
