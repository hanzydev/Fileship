<template>
    <main>
        <div v-if="currentUser">
            <Transition
                enter-active-class="motion-safe:(animate-in fade-in zoom-in-95 slide-in-top-2)"
                leave-active-class="motion-safe:(animate-out fade-out zoom-out-95 slide-out-top-2)"
            >
                <div v-if="!currentUser.backupRestoreState" ref="dashboard" relative>
                    <Transition
                        enter-active-class="motion-safe:(animate-in fade-in zoom-in-95)"
                        leave-active-class="motion-safe:(animate-out fade-out zoom-out-95)"
                    >
                        <div
                            v-if="isOverDropZone"
                            flex="~ items-center justify-center col gap4"
                            bg="black/60"
                            border="~ 2 dashed fs-accent"
                            h="[calc(100%-1.5rem)]"
                            w="[calc(100%-3rem)]"
                            absolute
                            left-6
                            top-6
                            z-9999
                            rounded-2xl
                            backdrop-blur-sm
                        >
                            <Icon name="solar:cloud-upload-bold" size="40" />
                            <h5>Drag and drop files here to upload</h5>
                        </div>
                    </Transition>

                    <LayoutsImpersonationBanner />

                    <div flex="~">
                        <LayoutsSidebar />

                        <slot />

                        <LayoutsUploadingFiles />
                    </div>
                </div>
            </Transition>
            <LayoutsBackupRestoring
                :open="!!currentUser.backupRestoreState"
                :state="currentUser.backupRestoreState!"
            />
        </div>
    </main>
</template>

<script setup lang="ts">
const embed = useEmbed();
const domains = useDomains();
const currentUser = useAuthUser();
const uploading = useIsUploading();
const uploadingFiles = useUploadingFiles();
const route = useRoute();
const { $toast } = useNuxtApp();
const { copy } = useClipboard({ legacy: true });

const dashboardRef = useTemplateRef('dashboard');

const handleUpload = async (files: File[] | null, source: 'drag-drop' | 'paste') => {
    if (!files?.length) return;

    uploading.value = true;

    const filesCopy = [...files];

    uploadingFiles.value = [...uploadingFiles.value, ...filesCopy];

    uploadingFiles.value.forEach((file) => {
        if (!file.status) {
            file.status = reactive({
                started: false,
                completed: false,
                progress: { speed: 0, percent: 0, eta: 0 },
                error: null,
            });
        }
    });

    const uploadedUrls: string[] = [];
    const parallelUploads = 3;

    const uploadResults = await uploadFilesWithConcurrency(
        filesCopy,
        (file) => uploadFile(file),
        parallelUploads,
    );
    const results = uploadResults.map((result) => typeof result === 'string');
    uploadedUrls.push(
        ...uploadResults.filter((result): result is string => typeof result === 'string'),
    );

    uploadingFiles.value = uploadingFiles.value.filter((_, index) => !results[index]);
    uploading.value = false;

    if (!uploadingFiles.value.length) {
        const filesCount = files.length;

        const isDragDrop = source === 'drag-drop';
        const filesText = filesCount > 1 ? `${filesCount} files` : 'File';
        const urlText = filesCount > 1 ? 'URLs' : 'URL';

        if (document.hasFocus()) {
            await copy(uploadedUrls.join('\n'));

            $toast.success(
                isDragDrop
                    ? `${filesText} dropped and uploaded successfully.  ${urlText} copied to clipboard!`
                    : `Pasted content uploaded successfully. ${urlText} copied to clipboard!`,
            );
        } else {
            $toast.success(
                isDragDrop
                    ? `${filesText} dropped and uploaded successfully. ${urlText} will copy when you return.`
                    : `Pasted content uploaded successfully. ${urlText} will copy when you return.`,
            );

            let settled = false;
            const cleanupEvents: (() => void)[] = [];

            const executeTryCopy = async () => {
                if (settled || !document.hasFocus()) return;

                try {
                    await copy(uploadedUrls.join('\n'), true);

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
        }
    } else {
        $toast.error('Some files could not be uploaded');
    }
};

const { isOverDropZone } = useDropZone(
    computed(() => (route.path === '/dashboard/files/upload' ? null : dashboardRef.value)),
    {
        onDrop: (files) => handleUpload(files, 'drag-drop'),
        multiple: true,
        preventDefaultForUnhandled: false,
    },
);

useEventListener(window, 'paste', (event: ClipboardEvent) => {
    const items = event.clipboardData?.items;
    if (!items) return;

    const files: File[] = [];

    for (const item of items) {
        if (item.kind === 'file' && item.type.startsWith('image/')) {
            const file = item.getAsFile();
            if (file) files.push(file);
        }
    }

    if (files.length > 0) {
        if (route.path !== '/dashboard/files/upload') handleUpload(files, 'paste');
        else {
            const filesCopy = [...files];
            uploadingFiles.value = [...uploadingFiles.value, ...filesCopy];
        }
    }
});

onMounted(async () => {
    embed.value = await $fetch<IEmbed>('/api/users/@me/embed');
    domains.value = await $fetch<string[]>('/api/users/@me/domains');
});
</script>
