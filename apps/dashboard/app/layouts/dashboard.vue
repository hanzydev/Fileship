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
                        v-if="adminSessionId"
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

                        <div class="ml-auto flex items-center gap-2 px-4">
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
                            <UiButton
                                variant="ghost"
                                size="icon"
                                @click="
                                    colorMode.preference =
                                        colorMode.value === 'dark' ? 'light' : 'dark'
                                "
                            >
                                <ISun class="hidden dark:block" />
                                <IMoon class="dark:hidden" />
                            </UiButton>
                        </div>
                    </header>

                    <slot />
                </UiSidebarInset>
            </UiSidebarProvider>

            <CommandPalette v-model:open="commandPaletteOpen" />
            <ConfirmDialog />
            <VerifyDialog />
            <UploadSheet v-if="canUpload" ref="uploadSheet" />
            <UploadQueue />

            <Transition
                enter-active-class="animate-in fade-in duration-200"
                leave-active-class="animate-out fade-out duration-150"
            >
                <div
                    v-if="isOverDropZone && canUpload && !uploadSheetOpen"
                    class="pointer-events-none fixed inset-0 z-50 flex items-center justify-center bg-background/60 p-6 backdrop-blur-md"
                >
                    <div
                        class="flex w-full max-w-md animate-in flex-col items-center gap-4 rounded-4xl border-2 border-dashed border-primary/60 bg-card p-10 text-center shadow-2xl duration-300 zoom-in-95"
                    >
                        <div class="relative flex size-16 items-center justify-center">
                            <span
                                class="absolute inset-0 animate-ping rounded-full bg-primary/20"
                            />
                            <span
                                class="relative flex size-16 items-center justify-center rounded-full bg-primary/10 text-primary"
                            >
                                <IUpload class="size-7" />
                            </span>
                        </div>
                        <div class="flex flex-col gap-1">
                            <TextBlock variant="large">Drop files to upload</TextBlock>
                            <TextBlock variant="muted">
                                {{
                                    currentFolderName
                                        ? `They will be added to ${currentFolderName}.`
                                        : 'Links are copied to your clipboard when done.'
                                }}
                            </TextBlock>
                        </div>
                    </div>
                </div>
            </Transition>
        </div>
    </main>
</template>

<script setup lang="ts">
import { upperFirst } from 'scule';

const embed = useEmbed();
const domains = useDomains();
const currentUser = useAuthUser();
const labels = useBreadcrumbLabels();
const route = useRoute();
const router = useRouter();
const colorMode = useColorMode();
const uploadSheetOpen = useUploadSheet();
const { quickUpload } = useUploader();

const uploadSheet = useTemplateRef('uploadSheet');
const commandPaletteOpen = ref(false);
const isMac = ref(false);
const isReturning = ref(false);
const { adminSessionId, stopImpersonating } = useImpersonation();

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

const breadcrumbItems = computed(() => {
    const segments = route.path.split('/').filter(Boolean);
    let path = '';

    return segments.map((segment) => {
        path += `/${segment}`;

        const label =
            labels.value[segment] ||
            breadcrumbLabels[segment as never] ||
            upperFirst(segment.replace(/[-_]/g, ' '));

        return { to: path, label };
    });
});

const canUpload = computed(() => canUploadFiles(currentUser.value));

const currentFolderName = computed(() =>
    route.path.startsWith('/dashboard/folders/') ? breadcrumbItems.value.at(-1)!.label : null,
);

const handleStopImpersonating = async () => {
    isReturning.value = true;
    await stopImpersonating();
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

    embed.value = await $fetch<IEmbed>('/api/users/@me/embed');
    domains.value = await $fetch<string[]>('/api/users/@me/domains');
});
</script>
