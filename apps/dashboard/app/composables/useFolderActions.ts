import type { FolderData } from '~/types/folders';

export const useFolderActions = () => {
    const { $toast, $i18n } = useNuxtApp();
    const { t } = $i18n;
    const { copy } = useClipboard({ legacy: true });
    const { isEnabled, deleteConfirmation, notifyDeleted } = useTrash();

    const copyLink = (folder: FolderData) => {
        copy(folder.publicUrl!);
        $toast.success(t('actions.linkCopied'));
    };

    const togglePublic = async (folder: FolderData) => {
        const id = $toast.loading(
            t(folder.public ? 'actions.folder.makingPrivate' : 'actions.folder.makingPublic'),
        );

        try {
            await $fetch(`/api/folders/${folder.id}`, {
                method: 'PATCH',
                body: { public: !folder.public },
            });

            $toast.success(
                t(folder.public ? 'actions.folder.nowPrivate' : 'actions.folder.nowPublic'),
                { id },
            );
        } catch (error: any) {
            $toast.error(getErrorMessage(error), { id });
        }
    };

    const remove = async (folder: FolderData) => {
        const option = reactive({
            label: t(
                isEnabled.value
                    ? 'actions.folder.alsoTrashFiles'
                    : 'actions.folder.alsoDeleteFiles',
            ),
            value: true,
        });

        return askConfirmation({
            ...deleteConfirmation(folder.name),
            option,
            action: async () => {
                try {
                    await $fetch(`/api/folders/${folder.id}`, {
                        method: 'DELETE',
                        body: { deleteFilesToo: option.value },
                    });

                    notifyDeleted(
                        t(option.value ? 'actions.folder.labelWithFiles' : 'actions.folder.label'),
                        {
                            folders: [folder.id],
                        },
                    );
                } catch (error: any) {
                    $toast.error(getErrorMessage(error));
                    return false;
                }
            },
        });
    };

    return { copyLink, togglePublic, download: downloadFolder, remove };
};
