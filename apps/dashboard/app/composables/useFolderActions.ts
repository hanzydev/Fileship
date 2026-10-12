export const useFolderActions = () => {
    const { $toast } = useNuxtApp();
    const { copy } = useClipboard({ legacy: true });
    const { isEnabled, deleteConfirmation, notifyDeleted } = useTrash();

    const copyLink = (folder: FolderData) => {
        copy(folder.publicUrl!);
        $toast.success('Link copied to clipboard');
    };

    const togglePublic = async (folder: FolderData) => {
        const id = $toast.loading(
            folder.public ? 'Making folder private...' : 'Making folder public...',
        );

        try {
            await $fetch(`/api/folders/${folder.id}`, {
                method: 'PATCH',
                body: { public: !folder.public },
            });

            $toast.success(folder.public ? 'Folder is now private' : 'Folder is now public', {
                id,
            });
        } catch (error: any) {
            $toast.error(getErrorMessage(error), { id });
        }
    };

    const remove = async (folder: FolderData) => {
        const option = reactive({
            label: isEnabled.value
                ? 'Also move the files inside to trash'
                : 'Also delete the files inside',
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

                    notifyDeleted(option.value ? 'Folder and its files' : 'Folder', {
                        folders: [folder.id],
                    });
                } catch (error: any) {
                    $toast.error(getErrorMessage(error));
                    return false;
                }
            },
        });
    };

    return { copyLink, togglePublic, download: downloadFolder, remove };
};
