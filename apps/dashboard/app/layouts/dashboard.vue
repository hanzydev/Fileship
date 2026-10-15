<template>
    <main>
        <div v-if="currentUser">
            <Centered v-if="currentUser.backupRestoreState">
                <UiCard class="w-full max-w-sm">
                    <UiCardHeader class="items-center text-center">
                        <UiSpinner class="mx-auto size-8" />
                        <UiCardTitle>Restoring your backup</UiCardTitle>
                        <UiCardDescription>
                            {{ restoreStates[currentUser.backupRestoreState] }}
                        </UiCardDescription>
                    </UiCardHeader>
                </UiCard>
            </Centered>

            <UiSidebarProvider v-else storage-key="sidebar">
                <AppSidebar />
                <UiSidebarInset>
                    <div
                        v-if="currentUser.impersonator"
                        class="flex flex-wrap items-center justify-center gap-x-4 gap-y-2 bg-primary px-4 py-2 text-sm text-primary-foreground"
                    >
                        <span class="flex items-center gap-2">
                            <IVenetianMask class="size-4" />
                            You are signed in as
                            <strong>{{ currentUser.username }}</strong>
                        </span>
                        <UiButton
                            size="xs"
                            variant="secondary"
                            :disabled="isReturning"
                            @click="handleStopImpersonating"
                        >
                            <UiSpinner v-if="isReturning" />
                            Return to my account
                        </UiButton>
                    </div>
                    <AppHeader :labels="breadcrumbLabels">
                        <UiButton v-if="canUpload" @click="uploadSheetOpen = true">
                            <IUpload />
                            <span class="hidden sm:inline">Upload</span>
                        </UiButton>
                        <UiButton
                            variant="outline"
                            class="hidden w-56 justify-start sm:flex"
                            @click="commandPaletteOpen = true"
                        >
                            <ISearch class="text-muted-foreground" />
                            <span class="font-normal text-muted-foreground">Search...</span>
                            <UiKbdGroup class="ml-auto">
                                <UiKbd>{{ isMac ? '⌘' : 'Ctrl' }}</UiKbd>
                                <UiKbd>K</UiKbd>
                            </UiKbdGroup>
                        </UiButton>
                        <UiButton
                            variant="ghost"
                            size="icon"
                            class="sm:hidden"
                            @click="commandPaletteOpen = true"
                        >
                            <ISearch />
                        </UiButton>
                    </AppHeader>

                    <slot />
                </UiSidebarInset>
            </UiSidebarProvider>

            <CommandPalette v-model:open="commandPaletteOpen" />
            <ConfirmDialog />
            <VerifyDialog />
            <UploadSheet v-if="canUpload" ref="uploadSheet" />
            <UploadQueue />

            <Transition
                enter-active-class="animate-in fade-in-0 duration-100 *:animate-in *:zoom-in-95 *:duration-100"
                leave-active-class="animate-out fade-out-0 duration-100 *:animate-out *:zoom-out-95 *:duration-100"
            >
                <div
                    v-if="isOverDropZone && canUpload && !uploadSheetOpen"
                    class="pointer-events-none fixed inset-0 z-50 flex items-center justify-center bg-black/30 p-4 supports-backdrop-filter:backdrop-blur-sm"
                >
                    <UiCard class="w-full max-w-sm">
                        <UiCardContent>
                            <UiEmpty class="border border-dashed">
                                <UiEmptyHeader>
                                    <UiEmptyMedia variant="icon">
                                        <IUpload />
                                    </UiEmptyMedia>
                                    <UiEmptyTitle>Drop files to upload</UiEmptyTitle>
                                    <UiEmptyDescription>
                                        {{
                                            currentFolderName
                                                ? `They will be added to ${currentFolderName}.`
                                                : 'Links are copied to your clipboard when done.'
                                        }}
                                    </UiEmptyDescription>
                                </UiEmptyHeader>
                            </UiEmpty>
                        </UiCardContent>
                    </UiCard>
                </div>
            </Transition>
        </div>
    </main>
</template>

<script setup lang="ts">
const embed = useEmbed();
const domains = useDomains();
const currentUser = useAuthUser();
const route = useRoute();
const { $toast } = useNuxtApp();
const router = useRouter();
const uploadSheetOpen = useUploadSheet();
const { quickUpload } = useUploader();

const uploadSheet = useTemplateRef('uploadSheet');
const commandPaletteOpen = ref(false);
const isMac = ref(false);
const isReturning = ref(false);
const { stopImpersonating } = useImpersonation();

const restoreStates = {
    Extracting: 'Extracting the archive, this may take a while...',
    DeletingPreviousData: 'Deleting previous data...',
    RestoringData: 'Restoring data...',
};

const breadcrumbLabels = {
    dashboard: 'Home',
    ai: 'AI',
    files: 'Files',
    folders: 'Folders',
    logs: 'Logs',
    notes: 'Notes',
    settings: 'Settings',
    stats: 'Statistics',
    users: 'Users',
};

const breadcrumbItems = useBreadcrumbs(breadcrumbLabels);

const canUpload = computed(() => canUploadFiles(currentUser.value));

const currentFolderName = computed(() =>
    route.path.startsWith('/dashboard/folders/') ? breadcrumbItems.value.at(-1)!.label : null,
);

const handleStopImpersonating = async () => {
    isReturning.value = true;
    await stopImpersonating().catch((error) => $toast.error(getErrorMessage(error)));
    isReturning.value = false;
};

const isTyping = () =>
    !!document.activeElement?.closest('input, textarea, select, [contenteditable="true"]');

const handleFiles = (files: File[] | null, source: 'drag-drop' | 'paste') => {
    if (!files?.length || !canUpload.value) return;

    if (uploadSheetOpen.value) uploadSheet.value!.addFiles(files);
    else quickUpload(files, source);
};

const { isOverDropZone } = useDropZone(import.meta.client ? document.body : null, {
    onDrop: (files) => handleFiles(files, 'drag-drop'),
    checkValidity: (items) => [...items].some((item) => item.kind === 'file'),
    multiple: true,
    preventDefaultForUnhandled: false,
});

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

    handleFiles(files, 'paste');
});

onKeyStroke(['u', 'U'], (event) => {
    if (event.ctrlKey || event.metaKey || event.altKey || isTyping() || !canUpload.value) return;

    event.preventDefault();
    uploadSheetOpen.value = true;
});

watch(
    () => route.query.upload,
    (value) => {
        if (value !== '1') return;

        uploadSheetOpen.value = true;
        router.replace({ query: { ...route.query, upload: undefined } });
    },
    { immediate: true },
);

onMounted(async () => {
    isMac.value = /Mac|iPhone|iPad/.test(navigator.userAgent);

    await Promise.all([
        $fetch<IEmbed>('/api/users/@me/embed').then((data) => (embed.value = data)),
        $fetch<string[]>('/api/users/@me/domains').then((data) => (domains.value = data)),
    ]).catch((error) => $toast.error(getErrorMessage(error)));
});
</script>
