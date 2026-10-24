import type { FolderData } from '~/types/folders';

export const useFolderActions = () => {
    const { $toast, $i18n } = useNuxtApp();
    const { t } = $i18n;
    const { copy } = useClipboard({ legacy: true });
    const { deleteConfirmation, notifyDeleted } = useTrash();

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

    const move = async (folder: FolderData, parentId: string | null) => {
        const id = $toast.loading(t('actions.folder.moving'));

        try {
            await $fetch(`/api/folders/${folder.id}`, { method: 'PATCH', body: { parentId } });
            $toast.success(t('actions.folder.moved'), { id });

            return true;
        } catch (error: any) {
            $toast.error(getErrorMessage(error), { id });
            return false;
        }
    };

    const remove = async (folder: FolderData) => {
        const option = reactive({ label: t('actions.folder.alsoDeleteFiles'), value: true });

        return askConfirmation({
            ...deleteConfirmation(folder.name, async (permanent) => {
                try {
                    await $fetch(`/api/folders/${folder.id}`, {
                        method: 'DELETE',
                        body: { deleteFilesToo: option.value, permanent },
                    });

                    notifyDeleted(
                        t(option.value ? 'actions.folder.labelWithFiles' : 'actions.folder.label'),
                        { folders: [folder.id] },
                        permanent,
                    );
                } catch (error: any) {
                    $toast.error(getErrorMessage(error));
                    return false;
                }
            }),
            option,
        });
    };

    return { copyLink, togglePublic, download: downloadFolder, move, remove };
};
