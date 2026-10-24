const DUPLICATE_CHECK_BATCH = 100;
const DUPLICATE_HASH_MAX_SIZE = 256 * 1024 * 1024;

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

    const hashContent = async (file: File) =>
        Array.from(
            new Uint8Array(await crypto.subtle.digest('SHA-256', await file.arrayBuffer())),
            (byte) => byte.toString(16).padStart(2, '0'),
        ).join('');

    const checkDuplicates = (files: { size: number; hash?: string }[]) =>
        $fetch('/api/files/duplicates', { method: 'POST', body: { files } }).then(
            (data) => data.files,
        );

    const findDuplicates = async (files: File[]) => {
        const originals = new Map<File, { id: string; name: string }>();
        const unchecked = new Set<File>();
        const comparable = files.filter((file) => !isProcessableImage(file.type));

        for (let i = 0; i < comparable.length; i += DUPLICATE_CHECK_BATCH) {
            const batch = comparable.slice(i, i + DUPLICATE_CHECK_BATCH);
            const sizes = await checkDuplicates(batch.map((file) => ({ size: file.size })));
            const candidates = batch.filter((_, index) => sizes[index]!.needsHash);
            const hashable = candidates.filter(
                (file) => window.isSecureContext && file.size <= DUPLICATE_HASH_MAX_SIZE,
            );

            candidates
                .filter((file) => !hashable.includes(file))
                .forEach((file) => unchecked.add(file));

            if (!hashable.length) continue;

            const hashes: string[] = [];
            for (const file of hashable) hashes.push(await hashContent(file));

            const results = await checkDuplicates(
                hashable.map((file, index) => ({ size: file.size, hash: hashes[index] })),
            );

            results.forEach(
                (result, index) =>
                    result.original && originals.set(hashable[index]!, result.original),
            );
        }

        return { originals, unchecked };
    };

    const confirmDuplicates = async (
        files: File[],
        originals: Map<File, { id: string; name: string }>,
    ) => {
        if (!originals.size) return files;

        const [file, original] = [...originals][0]!;
        const fresh = files.filter((f) => !originals.has(f));
        let selected = files;

        const isConfirmed = await askConfirmation({
            title: t('upload.duplicateTitle', originals.size),
            description:
                originals.size > 1
                    ? t('upload.duplicatesDescription')
                    : t('upload.duplicateDescription', {
                          name: file.name,
                          original: original.name,
                      }),
            confirmText: t('upload.uploadAnyway'),
            secondary: fresh.length
                ? { text: t('upload.skipDuplicates'), action: async () => (selected = fresh) }
                : undefined,
        });

        return isConfirmed ? selected : [];
    };

    const start = async (newFiles: UploadingFileData[]) => {
        warnAboutSpace(newFiles);
        uploading.value = true;

        newFiles.forEach((file) => {
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
                ...file.settings,
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

    const upload = async (files: File[], settings: FileSettings = {}) => {
        const { originals, unchecked } = await findDuplicates(files).catch(() => ({
            originals: new Map(),
            unchecked: new Set<File>(),
        }));
        const selected = (await confirmDuplicates(files, originals)) as UploadingFileData[];

        selected.forEach((file) => {
            file.settings = {
                ...settings,
                reportDuplicate: isProcessableImage(file.type) || unchecked.has(file),
            };
        });

        return start(selected);
    };

    const copyUrls = async (urls: string[], message: string) => {
        if (!urls.length) return;

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
            source === 'drag-drop' ? t('upload.dropped', urls.length) : t('upload.pasted'),
        );
    };

    const retry = (file: UploadingFileData) => {
        uploadingFiles.value = uploadingFiles.value.filter((f) => f !== file);
        return start([file]);
    };

    const dismiss = (file: UploadingFileData) => {
        uploadingFiles.value = uploadingFiles.value.filter((f) => f !== file);
        uploading.value = uploadingFiles.value.some((f) => !f.status!.error);
    };

    return { currentFolder, upload, copyUrls, quickUpload, retry, dismiss };
};
