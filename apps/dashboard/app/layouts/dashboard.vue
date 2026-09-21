<template>
    <main>
        <div v-if="currentUser">
            <!-- TODO: backup restoring layout -->
            <!-- TODO: impersonation banner -->
            <!-- TODO: drop zone -->
            <!-- TODO: uploading files right bottom -->

            <UiSidebarProvider storage-key="sidebar">
                <AppSidebar />
                <UiSidebarInset>
                    <header
                        class="flex h-16 shrink-0 items-center gap-2 transition-[width,height] ease-linear group-has-data-[collapsible=icon]/sidebar-wrapper:h-12"
                    >
                        <div class="flex items-center gap-2 px-4">
                            <UiSidebarTrigger class="-ml-1" />

                            <UiBreadcrumb>
                                <UiBreadcrumbList>
                                    <template
                                        v-for="(item, index) in breadcrumbItems"
                                        :key="item.to"
                                    >
                                        <UiBreadcrumbSeparator v-if="index > 0" />
                                        <UiBreadcrumbItem>
                                            <UiBreadcrumbLink
                                                v-if="index < breadcrumbItems.length - 1"
                                                as-child
                                            >
                                                <NuxtLink :to="item.to">{{ item.label }}</NuxtLink>
                                            </UiBreadcrumbLink>
                                            <UiBreadcrumbPage v-else>
                                                {{ item.label }}
                                            </UiBreadcrumbPage>
                                        </UiBreadcrumbItem>
                                    </template>
                                </UiBreadcrumbList>
                            </UiBreadcrumb>
                        </div>
                    </header>

                    <slot />
                </UiSidebarInset>
            </UiSidebarProvider>
        </div>
    </main>
</template>

<script setup lang="ts">
import { upperFirst } from 'scule';

const embed = useEmbed();
const domains = useDomains();
const currentUser = useAuthUser();
const uploading = useIsUploading();
const uploadingFiles = useUploadingFiles();
const route = useRoute();
const { $toast } = useNuxtApp();
const { copy } = useClipboard({ legacy: true });

// TODO: drop zone ui

const breadcrumbLabels = {
    dashboard: 'Home',
    account: 'Account',
    backups: 'Backups',
    files: 'Files',
    folders: 'Folders',
    logs: 'Logs',
    notes: 'Notes',
    sessions: 'Sessions',
    stats: 'Statistics',
    upload: 'Upload',
    users: 'Users',
};

const breadcrumbItems = computed(() => {
    const segments = route.path.split('/').filter(Boolean);
    let path = '';

    return segments.map((segment) => {
        path += `/${segment}`;

        return {
            to: path,
            label: breadcrumbLabels[segment as never] || upperFirst(segment.replace(/[-_]/g, ' ')),
        };
    });
});

// const dashboardRef = useTemplateRef('dashboard');

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
    const successfulFiles = new Set<File>();
    const parallelUploads = 3;

    await uploadFilesWithConcurrency(
        filesCopy,
        async (file) => {
            const res = await uploadFile(file);

            if (typeof res === 'string') {
                successfulFiles.add(file);
                uploadedUrls.push(res);
            }

            return res;
        },
        parallelUploads,
    );

    uploadingFiles.value = uploadingFiles.value.filter((f) => !successfulFiles.has(f));

    if (!uploadingFiles.value.length) {
        uploading.value = false;
    }

    if (successfulFiles.size === filesCopy.length) {
        const filesCount = filesCopy.length;

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
                    await copy(uploadedUrls.join('\n'));

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

/*const { isOverDropZone } = useDropZone(
    computed(() => (route.path === '/dashboard/files/upload' ? null : dashboardRef.value)),
    {
        onDrop: (files) => handleUpload(files, 'drag-drop'),
        multiple: true,
        preventDefaultForUnhandled: false,
    },
);*/

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
