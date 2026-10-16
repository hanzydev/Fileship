import type { FolderData } from '~/types/folders';

export const downloadFolder = async (folder: FolderData) => {
    const { $toast, $i18n } = useNuxtApp();
    const id = $toast.loading($i18n.t('actions.folder.preparingArchive'));

    try {
        const { jobId } = await $fetch(`/api/folders/${folder.id}/archive`, {
            method: 'POST',
        });

        const handleDownload = () => {
            downloadFile(`/api/folders/${folder.id}/archive/jobs/${jobId}/download`);
            $toast.success($i18n.t('actions.folder.archiveReady'), { id });
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
                $toast.error(getErrorMessage(e), { id });
            }
        }, 2_000);
    } catch (error: any) {
        $toast.error(getErrorMessage(error), { id });
    }
};
