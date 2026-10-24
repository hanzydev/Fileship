export const useUploader = () => {
    const route = useRoute();
    const uploading = useIsUploading();
    const uploadingFiles = useUploadingFiles();
    const storageUsage = useStorageUsage();
    const { $toast, $i18n } = useNuxtApp();
    const { t, locale } = $i18n;
    const { copy } = useClipboard({ legacy: true });

    const currentFolder = computed(() =>
        route.path.startsWith('/dashboard/folders/') ? ((route.params as any).id as string) : null,
    );

    const warnAboutSpace = (files: File[]) => {
        if (!storageUsage.value?.limit) return;

        const needed = files.reduce((sum, file) => sum + file.size, 0);
        const left = Math.max(
            0,
            Number(storageUsage.value.limit) - Number(storageUsage.value.used),
        );
        if (needed <= left) return;

        $toast.warning(t('upload.notEnoughSpace'), {
            description: t('upload.notEnoughSpaceDescription', {
                needed: formatStorageSize(needed, locale.value),
                left: formatStorageSize(left, locale.value),
            }),
        });
    };

    const upload = async (files: File[], settings: FileSettings = {}) => {
        warnAboutSpace(files);
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

        const results = await uploadFilesWithConcurrency(newFiles, async (file) => {
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

        return { urls, results, failed: newFiles.length - successfulFiles.size };
    };

    const copyUrls = async (urls: string[], message: string) => {
        if (document.hasFocus()) {
            await copy(urls.join('\n'));
            $toast.success(message, { description: t('upload.urlCopied', urls.length) });
            return;
        }

        $toast.success(message, { description: t('upload.urlCopyOnReturn', urls.length) });

        let settled = false;
        const cleanupEvents: (() => void)[] = [];

        const executeTryCopy = async () => {
            if (settled || !document.hasFocus()) return;

            try {
                await copy(urls.join('\n'));

                settled = true;
                cleanupEvents.forEach((cleanup) => cleanup());
                $toast.success(t('upload.urlCopied', urls.length));
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
            $toast.error(t('upload.someFailed'));
            return;
        }

        await copyUrls(
            urls,
            source === 'drag-drop' ? t('upload.dropped', files.length) : t('upload.pasted'),
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
