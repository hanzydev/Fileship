export const useFileActions = () => {
    const embed = useEmbed();
    const { $toast } = useNuxtApp();
    const { copy } = useClipboard({ legacy: true });

    const isTextFile = (file: FileData) =>
        TEXT_FILE_TYPES.some((type) => `.${type.extension}` === getExtname(file.fileName));

    const copyLink = (file: FileData) => {
        copy(embed.value.enabled || isTextFile(file) ? file.embedUrl : file.directUrl);
        $toast.success('Link copied to clipboard');
    };

    const openFile = (file: FileData) => window.open(file.embedUrl, '_blank');

    const download = (files: FileData[]) =>
        files.forEach((file) => downloadFile(`${file.directUrl}?download`));

    const move = async (files: Pick<FileData, 'id'>[], folderId: string | null) => {
        try {
            if (files.length > 1) {
                await $fetch('/api/files/bulk-move', {
                    method: 'POST',
                    body: { files: files.map((f) => f.id), folderId },
                });
            } else {
                await $fetch(`/api/files/${files[0]!.id}`, {
                    method: 'PATCH',
                    body: { folderId },
                });
            }

            const filesText = files.length > 1 ? `${files.length} files` : 'File';
            $toast.success(folderId ? `${filesText} moved` : `${filesText} removed from folder`);

            return true;
        } catch (error: any) {
            $toast.error(error.data.message);
            return false;
        }
    };

    const remove = async (files: Pick<FileData, 'id' | 'fileName'>[]) => {
        const filesText = files.length > 1 ? `${files.length} files` : files[0]!.fileName;

        const confirmed = await askConfirmation({
            title: `Delete ${filesText}?`,
            description: 'This action cannot be undone.',
            confirmText: 'Delete',
            destructive: true,
        });

        if (!confirmed) return false;

        try {
            if (files.length > 1) {
                await $fetch('/api/files/bulk-delete', {
                    method: 'POST',
                    body: { files: files.map((f) => f.id) },
                });
            } else {
                await $fetch(`/api/files/${files[0]!.id}`, { method: 'DELETE' });
            }

            $toast.success(`${files.length > 1 ? `${files.length} files` : 'File'} deleted`);

            return true;
        } catch (error: any) {
            $toast.error(error.data.message);
            return false;
        }
    };

    return { isTextFile, copyLink, openFile, download, move, remove };
};
