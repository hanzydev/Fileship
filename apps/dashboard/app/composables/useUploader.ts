export const useUploader = () => {
    const route = useRoute();
    const uploading = useIsUploading();
    const uploadingFiles = useUploadingFiles();
    const { $toast } = useNuxtApp();
    const { copy } = useClipboard({ legacy: true });

    const currentFolder = computed(() =>
        route.path.startsWith('/dashboard/folders/') ? ((route.params as any).id as string) : null,
    );

    const upload = async (files: File[], settings: FileSettings = {}) => {
        uploading.value = true;

        const newFiles = files as UploadingFileData[];
        newFiles.forEach((file) => {
            file.settings = settings;
            file.status = reactive({
                started: false,
                completed: false,
                progress: { speed: 0, percent: 0, eta: 0 },
                error: null,
            });
        });

        uploadingFiles.value = [...uploadingFiles.value, ...newFiles];

        const urls: string[] = [];
        const successfulFiles = new Set<File>();

        await uploadFilesWithConcurrency(newFiles, async (file) => {
            const res = await uploadFile(file, {
                fileNameType: 'Random',
                maxViews: 0,
                compression: 0,
                ...settings,
            });

            if (typeof res === 'string') {
                successfulFiles.add(file);
                urls.push(res);
            }

            return res;
        });

        uploadingFiles.value = uploadingFiles.value.filter((f) => !successfulFiles.has(f));
        uploading.value = uploadingFiles.value.some((f) => !f.status!.error);

        return { urls, failed: newFiles.length - successfulFiles.size };
    };

    const copyUrls = async (urls: string[], message: string) => {
        const urlText = urls.length > 1 ? 'URLs' : 'URL';

        if (document.hasFocus()) {
            await copy(urls.join('\n'));
            $toast.success(`${message} ${urlText} copied to clipboard!`);
            return;
        }

        $toast.success(`${message} ${urlText} will copy when you return.`);

        let settled = false;
        const cleanupEvents: (() => void)[] = [];

        const executeTryCopy = async () => {
            if (settled || !document.hasFocus()) return;

            try {
                await copy(urls.join('\n'));

                settled = true;
                cleanupEvents.forEach((cleanup) => cleanup());
                $toast.success(`${urlText} copied to clipboard!`);
            } catch {
                //
            }
        };

        cleanupEvents.push(useEventListener(window, 'click', executeTryCopy));
        cleanupEvents.push(useEventListener(window, 'mousedown', executeTryCopy));
        cleanupEvents.push(useEventListener(window, 'touchend', executeTryCopy));
        cleanupEvents.push(useEventListener(window, 'keydown', executeTryCopy));
    };

    const quickUpload = async (files: File[], source: 'drag-drop' | 'paste') => {
        const { urls, failed } = await upload(files, { folder: currentFolder.value });

        if (failed) {
            $toast.error('Some files could not be uploaded');
            return;
        }

        const filesText = files.length > 1 ? `${files.length} files` : 'File';

        await copyUrls(
            urls,
            source === 'drag-drop'
                ? `${filesText} dropped and uploaded successfully.`
                : 'Pasted content uploaded successfully.',
        );
    };

    const retry = (file: UploadingFileData) => {
        uploadingFiles.value = uploadingFiles.value.filter((f) => f !== file);
        return upload([file], file.settings);
    };

    const dismiss = (file: UploadingFileData) => {
        uploadingFiles.value = uploadingFiles.value.filter((f) => f !== file);
        uploading.value = uploadingFiles.value.some((f) => !f.status!.error);
    };

    return { currentFolder, upload, copyUrls, quickUpload, retry, dismiss };
};
