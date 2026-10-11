export const useFolderActions = () => {
    const { $toast } = useNuxtApp();
    const { copy } = useClipboard({ legacy: true });

    const copyLink = (folder: FolderData) => {
        copy(folder.publicUrl!);
        $toast.success('Link copied to clipboard');
    };

    const togglePublic = async (folder: FolderData) => {
        try {
            await $fetch(`/api/folders/${folder.id}`, {
                method: 'PATCH',
                body: { public: !folder.public },
            });

            $toast.success(folder.public ? 'Folder is now private' : 'Folder is now public');
        } catch (error: any) {
            $toast.error(error.data.message);
        }
    };

    const remove = async (folder: FolderData) => {
        const option = reactive({ label: 'Also delete the files inside', value: true });

        return askConfirmation({
            title: `Delete ${folder.name}?`,
            description: 'This action cannot be undone.',
            confirmText: 'Delete',
            destructive: true,
            option,
            action: async () => {
                try {
                    await $fetch(`/api/folders/${folder.id}`, {
                        method: 'DELETE',
                        body: { deleteFilesToo: option.value },
                    });

                    $toast.success(
                        option.value ? 'Folder and its files deleted' : 'Folder deleted',
                    );
                } catch (error: any) {
                    $toast.error(error.data.message);
                    return false;
                }
            },
        });
    };

    return { copyLink, togglePublic, download: downloadFolder, remove };
};
