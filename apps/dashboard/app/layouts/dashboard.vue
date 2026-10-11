<template>
    <main>
        <div v-if="currentUser">
            <!-- TODO: impersonation banner -->

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
                                class="hidden sm:flex"
                                @click="commandPaletteOpen = true"
                            >
                                <ISearch />
                                <span class="text-muted-foreground">Search...</span>
                                <UiKbdGroup class="ml-4">
                                    <UiKbd>Ctrl</UiKbd>
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
                enter-active-class="animate-in fade-in duration-150"
                leave-active-class="animate-out fade-out duration-150"
            >
                <div
                    v-if="isOverDropZone && canUpload && !uploadSheetOpen"
                    class="pointer-events-none fixed inset-0 z-50 flex items-center justify-center bg-background/80 p-6 backdrop-blur-sm"
                >
                    <div
                        class="flex size-full flex-col items-center justify-center gap-3 rounded-3xl border-2 border-dashed border-primary"
                    >
                        <IUpload class="size-10 text-primary" />
                        <TextBlock variant="h4" custom-tag="p">Drop to upload</TextBlock>
                        <TextBlock variant="muted">
                            Files are uploaded instantly and links are copied to your clipboard.
                        </TextBlock>
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
const notes = useNotes();
const folders = useFolders();
const route = useRoute();
const router = useRouter();
const colorMode = useColorMode();
const uploadSheetOpen = useUploadSheet();
const { quickUpload } = useUploader();

const uploadSheet = useTemplateRef('uploadSheet');
const commandPaletteOpen = ref(false);

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

    return segments.map((segment, index) => {
        path += `/${segment}`;

        const parent = segments[index - 1];
        const label =
            (parent === 'folders' && folders.value.find((f) => f.id === segment)?.name) ||
            (parent === 'notes' && notes.value.find((n) => n.id === segment)?.title) ||
            breadcrumbLabels[segment as never] ||
            upperFirst(segment.replace(/[-_]/g, ' '));

        return { to: path, label };
    });
});

const canUpload = computed(() => canUploadFiles(currentUser.value));

const isTyping = () =>
    !!document.activeElement?.closest('input, textarea, select, [contenteditable="true"]');

const handleFiles = (files: File[] | null, source: 'drag-drop' | 'paste') => {
    if (!files?.length || !canUpload.value) return;

    if (uploadSheetOpen.value) uploadSheet.value!.addFiles(files);
    else quickUpload(files, source);
};

const { isOverDropZone } = useDropZone(import.meta.client ? document.body : null, {
    onDrop: (files) => handleFiles(files, 'drag-drop'),
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
    embed.value = await $fetch<IEmbed>('/api/users/@me/embed');
    domains.value = await $fetch<string[]>('/api/users/@me/domains');
});
</script>
