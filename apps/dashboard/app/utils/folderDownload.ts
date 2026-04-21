export const downloadFolder = async (folder: FolderData) => {
    const { $toast } = useNuxtApp();
    const currentUser = useAuthUser();

    try {
        const { jobId } = await $fetch(`/api/folders/${folder.id}/archive`, {
            method: 'POST',
        });

        $toast.info('Preparing archive... Will download when ready.');

        const handleDownload = () => {
            window.open(`/api/folders/${folder.id}/archive/jobs/${jobId}/download`, '_blank');
        };

        if (currentUser.value?.id && currentUser.value.id === folder.authorId) {
            const socket = getSocket();
            if (socket) {
                const onArchiveReady = (folderId: string) => {
                    if (folderId === folder.id) {
                        handleDownload();
                        socket.off('folder:archiveReady', onArchiveReady);
                    }
                };
                socket.on('folder:archiveReady', onArchiveReady);
            }

            return;
        }

        const interval = setInterval(async () => {
            try {
                const statusRes = await $fetch(`/api/folders/${folder.id}/archive/jobs/${jobId}`);
                if (statusRes.isDone) {
                    clearInterval(interval);
                    handleDownload();
                }
            } catch (e: any) {
                clearInterval(interval);
                $toast.error(e.data.message);
            }
        }, 2_000);
    } catch (error: any) {
        $toast.error(error.data.message);
    }
};
