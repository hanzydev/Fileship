export const downloadFolder = async (folder: FolderData) => {
    const { $toast } = useNuxtApp();

    try {
        const { jobId } = await $fetch(`/api/folders/${folder.id}/archive`, {
            method: 'POST',
        });

        $toast.info('Preparing archive... Will download when ready.');

        const handleDownload = () => {
            downloadFile(`/api/folders/${folder.id}/archive/jobs/${jobId}/download`);
        };

        const interval = setInterval(async () => {
            try {
                const statusRes = await $fetch(`/api/folders/${folder.id}/archive/jobs/${jobId}`);
                if (statusRes.isDone) {
                    clearInterval(interval);
                    handleDownload();
                }
            } catch (e: any) {
                clearInterval(interval);
                $toast.error(getErrorMessage(e));
            }
        }, 2_000);
    } catch (error: any) {
        $toast.error(getErrorMessage(error));
    }
};
