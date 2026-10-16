import type { FileData } from '~/types/files';

export const useFileActions = () => {
    const embed = useEmbed();
    const { $toast, $i18n } = useNuxtApp();
    const { t } = $i18n;
    const { copy } = useClipboard({ legacy: true });
    const { deleteConfirmation, notifyDeleted } = useTrash();

    const isTextFile = (file: FileData) =>
        TEXT_FILE_TYPES.some((type) => `.${type.extension}` === getExtname(file.fileName));

    const copyLink = (file: FileData) => {
        copy(embed.value.enabled || isTextFile(file) ? file.embedUrl : file.directUrl);
        $toast.success(t('actions.linkCopied'));
    };

    const openFile = (file: FileData) => window.open(file.embedUrl, '_blank');

    const download = (files: FileData[]) =>
        files.forEach((file) => downloadFile(`${file.directUrl}?download`));

    const move = async (files: Pick<FileData, 'id'>[], folderId: string | null) => {
        const id = $toast.loading(
            folderId
                ? t('actions.file.moving', files.length)
                : t('actions.file.removingFromFolder'),
        );

        try {
            if (files.length > 1) {
                for (const chunk of chunkIds(files.map((file) => file.id))) {
                    await $fetch('/api/files/bulk-move', {
                        method: 'POST',
                        body: { files: chunk, folderId },
                    });
                }
            } else {
                await $fetch(`/api/files/${files[0]!.id}`, {
                    method: 'PATCH',
                    body: { folderId },
                });
            }

            $toast.success(
                t(folderId ? 'actions.file.moved' : 'actions.file.removedFromFolder', files.length),
                { id },
            );

            return true;
        } catch (error: any) {
            $toast.error(getErrorMessage(error), { id });
            return false;
        }
    };

    const remove = async (files: Pick<FileData, 'id' | 'name'>[]) => {
        const filesText = files.length > 1 ? t('actions.file.count', files.length) : files[0]!.name;

        return askConfirmation({
            ...deleteConfirmation(filesText),
            action: async () => {
                try {
                    if (files.length > 1) {
                        for (const chunk of chunkIds(files.map((file) => file.id))) {
                            await $fetch('/api/files/bulk-delete', {
                                method: 'POST',
                                body: { files: chunk },
                            });
                        }
                    } else {
                        await $fetch(`/api/files/${files[0]!.id}`, { method: 'DELETE' });
                    }

                    notifyDeleted(t('actions.file.label', files.length), {
                        files: files.map((file) => file.id),
                    });
                } catch (error: any) {
                    $toast.error(getErrorMessage(error));
                    return false;
                }
            },
        });
    };

    const setSensitive = async (file: Pick<FileData, 'id'>, sensitive: boolean) => {
        const id = $toast.loading(
            t(sensitive ? 'actions.file.markingSensitive' : 'actions.file.markingNotSensitive'),
        );

        try {
            await $fetch(`/api/files/${file.id}`, { method: 'PATCH', body: { sensitive } });
            $toast.success(
                t(sensitive ? 'actions.file.markedSensitive' : 'actions.file.markedNotSensitive'),
                { id },
            );
            return true;
        } catch (error: any) {
            $toast.error(getErrorMessage(error), { id });
            return false;
        }
    };

    return { isTextFile, copyLink, openFile, download, move, remove, setSensitive };
};
