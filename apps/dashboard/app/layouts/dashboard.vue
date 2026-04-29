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

const dashboardRef = useTemplateRef('dashboard');

const writeToClipboard = async (text: string): Promise<boolean> => {
    if (!import.meta.client) return false;

    try {
        const ta = document.createElement('textarea');
        ta.value = text;
        ta.setAttribute('readonly', '');
        ta.style.position = 'fixed';
        ta.style.top = '0';
        ta.style.left = '-9999px';
        ta.style.opacity = '0';
        document.body.appendChild(ta);

        const previouslyFocused = document.activeElement as HTMLElement | null;
        ta.focus();
        ta.select();
        ta.setSelectionRange(0, text.length);

        const ok = document.execCommand('copy');

        ta.remove();
        previouslyFocused?.focus?.();

        if (ok) return true;
    } catch {}

    try {
        if (navigator.clipboard?.writeText) {
            await navigator.clipboard.writeText(text);
            return true;
        }
    } catch {}

    return false;
};

const writeToClipboardWhenFocused = (text: string, timeoutMs = 120_000): Promise<boolean> => {
    if (!import.meta.client) return Promise.resolve(false);

    return new Promise((resolve) => {
        let settled = false;

        const finish = (ok: boolean) => {
            if (settled) return;
            settled = true;
            window.removeEventListener('focus', tryCopy);
            document.removeEventListener('visibilitychange', tryCopy);
            window.removeEventListener('pointerdown', tryCopy);
            window.removeEventListener('keydown', tryCopy);
            clearTimeout(timer);
            resolve(ok);
        };

        const tryCopy = async () => {
            if (settled) return;
            if (!document.hasFocus()) return;
            const ok = await writeToClipboard(text);
            if (ok) finish(true);
        };

        const timer = setTimeout(() => finish(false), timeoutMs);

        if (document.hasFocus()) {
            void tryCopy();
            if (settled) return;
        }

        window.addEventListener('focus', tryCopy);
        document.addEventListener('visibilitychange', tryCopy);
        window.addEventListener('pointerdown', tryCopy);
        window.addEventListener('keydown', tryCopy);
    });
};

const handleUpload = async (files: File[] | null, source: 'drag-drop' | 'paste') => {
    if (!files?.length) return;

    uploading.value = true;

    const filesCopy = [...files];

    uploadingFiles.value = [...uploadingFiles.value, ...filesCopy];

    uploadingFiles.value.forEach((file) => {
        if (!file.status) {
            file.status = reactive({
                started: false,
                progress: { speed: 0, percent: 0, eta: 0 },
                error: null,
            });
        }
    });

    const results: (boolean | { url: string })[] = [];
    const uploadedUrls: string[] = [];
    const parallelUploads = 3;

    while (filesCopy.length > 0) {
        const chunk = filesCopy.splice(0, parallelUploads);
        const chunkResults = await Promise.all(chunk.map((c) => uploadFile(c)));
        for (const r of chunkResults) {
            if (r === undefined) continue;
            results.push(r);
            if (r && typeof r === 'object' && r.url) uploadedUrls.push(r.url);
        }
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

        if (uploadedUrls.length) {
            writeToClipboardWhenFocused(uploadedUrls.join('\n')).then((copied) => {
                if (copied) {
                    $toast.success(
                        uploadedUrls.length > 1
                            ? `${uploadedUrls.length} links copied to clipboard`
                            : 'Link copied to clipboard',
                    );
                }
            });
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
