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

                    <LayoutsActingAsUser />

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

const dashboardRef = useTemplateRef('dashboard');

const handleUpload = async (files: File[] | null, source: 'drag-drop' | 'paste') => {
    if (!files?.length) return;

    uploading.value = true;

    const filesCopy = [...files];

    uploadingFiles.value = [...uploadingFiles.value, ...filesCopy];

    const results: boolean[] = [];
    const parallelUploads = 3;

    while (filesCopy.length > 0) {
        const chunk = filesCopy.splice(0, parallelUploads);
        const chunkResults = await Promise.all(chunk.map((c) => uploadFile(c)));
        results.push(...chunkResults.filter((r) => r !== undefined));
    }

    uploadingFiles.value = uploadingFiles.value.filter((_, index) => !results[index]);
    uploading.value = false;

    if (!uploadingFiles.value.length) {
        const filesCount = files.length;

        $toast.success(
            source === 'drag-drop'
                ? `${filesCount} file${filesCount > 1 ? 's' : ''} dropped and uploaded successfully`
                : 'Pasted content uploaded successfully',
        );
    } else {
        $toast.error('Some files could not be uploaded');
    }
};

const { isOverDropZone } = useDropZone(
    computed(() => (route.path === '/dashboard/files/upload' ? null : dashboardRef.value)),
    {
        onDrop: (files: File[]) => handleUpload(files, 'drag-drop'),
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
