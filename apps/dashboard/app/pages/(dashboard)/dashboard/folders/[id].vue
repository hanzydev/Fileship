<template>
    <Head>
        <Title>{{ folder?.name || 'Folder' }}</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-6 p-4 pt-0">
        <EmptyState
            v-if="!folder && !isLoading"
            :icon="IFolderX"
            title="Folder not found"
            description="It may have been deleted."
        >
            <UiButton variant="outline" as-child>
                <NuxtLink to="/dashboard/folders">Back to folders</NuxtLink>
            </UiButton>
        </EmptyState>

        <template v-else-if="folder">
            <PageHeader :title="folder.name" :description="description">
                <UiButton variant="outline" @click="addFilesOpen = true">
                    <IFilePlus />
                    Add files
                </UiButton>
                <UiButton v-if="folder.public" variant="outline" @click="copyLink(folder)">
                    <ILink />
                    Copy link
                </UiButton>
                <UiDropdownMenu>
                    <UiDropdownMenuTrigger as-child>
                        <UiButton variant="outline" size="icon">
                            <IEllipsis />
                        </UiButton>
                    </UiDropdownMenuTrigger>
                    <UiDropdownMenuContent align="end" class="w-56">
                        <FolderMenuItems
                            :folder="folder"
                            type="dropdown"
                            in-folder
                            @edit="formOpen = true"
                        />
                    </UiDropdownMenuContent>
                </UiDropdownMenu>
            </PageHeader>

            <UiAlert v-if="folder.isInbox">
                <IInbox />
                <UiAlertTitle>This is your inbox</UiAlertTitle>
                <UiAlertDescription>
                    Files that other people send you through your inbox link land here.
                    <NuxtLink to="/dashboard/settings/inbox" class="underline underline-offset-4">
                        Manage inbox settings
                    </NuxtLink>
                </UiAlertDescription>
            </UiAlert>

            <FileBrowser
                v-model:summary="summary"
                :folder-id="folder.id"
                empty-title="This folder is empty"
            />

            <FolderFormDialog v-model:open="formOpen" :folder="folder" />

            <UiDialog v-model:open="addFilesOpen">
                <UiDialogContent class="sm:max-w-lg">
                    <UiDialogHeader>
                        <UiDialogTitle>Add files to {{ folder.name }}</UiDialogTitle>
                        <UiDialogDescription>Pick files from your library.</UiDialogDescription>
                    </UiDialogHeader>
                    <UiInputGroup>
                        <UiInputGroupAddon>
                            <ISearch />
                        </UiInputGroupAddon>
                        <UiInputGroupInput v-model="addSearch" placeholder="Search files..." />
                    </UiInputGroup>
                    <div
                        class="-mx-1 flex max-h-80 flex-col gap-2 overflow-y-auto px-1 py-1"
                        @scroll="handleScroll"
                    >
                        <UiFieldLabel
                            v-for="file in addCandidates"
                            :key="file.id"
                            :for="`add-file-${file.id}`"
                        >
                            <UiField orientation="horizontal">
                                <UiCheckbox
                                    :id="`add-file-${file.id}`"
                                    :model-value="addSelection.includes(file.id)"
                                    @update:model-value="toggleAdd(file.id)"
                                />
                                <UiFieldContent class="min-w-0">
                                    <UiFieldTitle class="w-full truncate">
                                        {{ file.fileName }}
                                    </UiFieldTitle>
                                    <UiFieldDescription>
                                        {{ file.size.formatted }}
                                    </UiFieldDescription>
                                </UiFieldContent>
                            </UiField>
                        </UiFieldLabel>
                        <div v-if="isCandidatesLoading" class="flex justify-center py-6">
                            <UiSpinner class="text-muted-foreground" />
                        </div>
                        <TextBlock
                            v-else-if="!addCandidates.length"
                            variant="muted"
                            class="py-6 text-center"
                        >
                            No files to add.
                        </TextBlock>
                    </div>
                    <UiDialogFooter>
                        <UiButton variant="outline" @click="addFilesOpen = false">Cancel</UiButton>
                        <UiButton
                            :disabled="!addSelection.length || isAdding"
                            @click="handleAddFiles"
                        >
                            <UiSpinner v-if="isAdding" />
                            Add {{ pluralize(addSelection.length, 'file') }}
                        </UiButton>
                    </UiDialogFooter>
                </UiDialogContent>
            </UiDialog>
        </template>
    </div>
</template>

<script setup lang="ts">
import { IFolderX } from '#components';

const route = useRoute();
const labels = useBreadcrumbLabels();
const { $toast } = useNuxtApp();
const { copyLink } = useFolderActions();

const formOpen = ref(false);
const addFilesOpen = ref(false);
const addSearch = ref('');
const addSelection = ref<string[]>([]);
const isAdding = ref(false);
const summary = ref<FileSummary>();

const folderId = computed(() => (route.params as any).id as string);

const {
    data: folder,
    status,
    refresh,
} = useLazyAsyncData(
    () => `folder:${folderId.value}`,
    () => $fetch<FolderData>(`/api/folders/${folderId.value}`),
    { server: false },
);

const isLoading = computed(() => status.value === 'pending' || status.value === 'idle');

const {
    items: addCandidates,
    isLoading: isCandidatesLoading,
    loadMore,
} = usePaginatedList<FileData>(
    'files',
    '/api/files',
    () => ({ q: addSearch.value.trim(), excludeFolder: folderId.value }),
    { limit: 30, enabled: addFilesOpen },
);

const description = computed(
    () =>
        `${pluralize(summary.value?.count ?? folder.value!.fileCount, 'file')} · ${folder.value!.public ? 'Public' : 'Private'}`,
);

const toggleAdd = (id: string) => {
    addSelection.value = addSelection.value.includes(id)
        ? addSelection.value.filter((x) => x !== id)
        : [...addSelection.value, id];
};

const handleScroll = (event: Event) => {
    const target = event.target as HTMLElement;
    if (target.scrollHeight - target.scrollTop - target.clientHeight < 80) loadMore();
};

const handleAddFiles = async () => {
    isAdding.value = true;

    try {
        await $fetch('/api/files/bulk-move', {
            method: 'POST',
            body: { files: addSelection.value, folderId: folderId.value },
        });

        $toast.success(`${pluralize(addSelection.value.length, 'file')} added`);
        addFilesOpen.value = false;
    } catch (error: any) {
        $toast.error(getErrorMessage(error));
    }

    isAdding.value = false;
};

useListHandle({
    resource: 'folders',
    refresh,
    map: (fn) => folder.value && (folder.value = fn(folder.value)),
    remove: (ids) => ids.includes(folderId.value) && navigateTo('/dashboard/folders'),
});

watch(folder, (value) => value && (labels.value = { ...labels.value, [value.id]: value.name }));

watch(addFilesOpen, (value) => {
    if (value) {
        addSearch.value = '';
        addSelection.value = [];
    }
});

definePageMeta({
    layout: 'dashboard',
    middleware: 'file-uploader-only',
});
</script>
