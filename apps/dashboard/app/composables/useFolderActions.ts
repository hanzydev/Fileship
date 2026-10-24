import type { FolderData } from '~/types/folders';

export const useFolderActions = () => {
    const { $toast, $i18n } = useNuxtApp();
    const { t } = $i18n;
    const { copy } = useClipboard({ legacy: true });
    const { deleteConfirmation, notifyDeleted } = useTrash();
    const currentUser = useAuthUser();

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

    const leave = (folder: FolderData) =>
        askConfirmation({
            title: t('actions.folder.leaveTitle', { name: folder.name }),
            description: t('actions.folder.leaveDescription'),
            confirmText: t('actions.folder.leave'),
            destructive: true,
            action: async () => {
                try {
                    await $fetch(`/api/folders/${folder.id}/members/${currentUser.value!.id}`, {
                        method: 'DELETE',
                    });
                    $toast.success(t('actions.folder.left'));
                } catch (error: any) {
                    $toast.error(getErrorMessage(error));
                    return false;
                }
            },
        });

    const remove = async (folder: FolderData) => {
        const option = reactive({ label: t('actions.folder.alsoDeleteFiles'), value: true });
        const isShared = !isOwnedItem(folder);

        return askConfirmation({
            ...deleteConfirmation(
                folder.name,
                async (permanent) => {
                    try {
                        await $fetch(`/api/folders/${folder.id}`, {
                            method: 'DELETE',
                            body: { deleteFilesToo: option.value, permanent },
                        });

                        notifyDeleted(
                            t(
                                option.value
                                    ? 'actions.folder.labelWithFiles'
                                    : 'actions.folder.label',
                            ),
                            { folders: [folder.id] },
                            permanent,
                            isShared,
                        );
                    } catch (error: any) {
                        $toast.error(getErrorMessage(error));
                        return false;
                    }
                },
                isShared,
            ),
            option,
        });
    };

    return { copyLink, togglePublic, download: downloadFolder, move, leave, remove };
};
