<template>
    <Head>
        <Title>{{ folder?.name || 'Folder' }}</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-4 p-4 pt-0">
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
                <UiButton variant="outline" @click="pickerOpen = true">
                    <IFilePlus />
                    Manage files
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

            <UiDialog v-model:open="pickerOpen">
                <UiDialogContent class="flex h-[min(85svh,52rem)] flex-col sm:max-w-4xl">
                    <UiDialogHeader>
                        <UiDialogTitle>Files in {{ folder.name }}</UiDialogTitle>
                        <UiDialogDescription>
                            Select the files that belong in this folder. Files from other folders
                            are moved here.
                        </UiDialogDescription>
                    </UiDialogHeader>

                    <div class="flex flex-wrap gap-2">
                        <UiTabs v-model="pickerFilter">
                            <UiTabsList>
                                <UiTabsTrigger
                                    v-for="filter in pickerFilters"
                                    :key="filter.value"
                                    :value="filter.value"
                                >
                                    {{ filter.label }}
                                </UiTabsTrigger>
                            </UiTabsList>
                        </UiTabs>
                        <UiInputGroup class="min-w-48 flex-1">
                            <UiInputGroupAddon>
                                <UiSpinner v-if="!!pickerSearch && isPickerLoading" />
                                <ISearch v-else />
                            </UiInputGroupAddon>
                            <UiInputGroupInput
                                v-model="pickerSearch"
                                placeholder="Search files..."
                            />
                        </UiInputGroup>
                    </div>

                    <div class="-mx-2 min-h-0 flex-1 overflow-y-auto px-2 pt-1">
                        <EmptyState
                            v-if="isPickerError"
                            :icon="ICircleAlert"
                            title="Couldn't load files"
                            description="Something went wrong. Check your connection and try again."
                        >
                            <UiButton variant="outline" @click="refreshPicker">Try again</UiButton>
                        </EmptyState>
                        <EmptyState
                            v-else-if="!isPickerLoading && !pickerFiles.length"
                            :icon="IFiles"
                            title="No files here"
                            description="Try another filter or search."
                        />
                        <div
                            v-else-if="isPickerLoading"
                            class="grid grid-cols-[repeat(auto-fill,minmax(160px,1fr))] gap-3 pb-2"
                        >
                            <FileCardSkeleton
                                v-for="i in 8"
                                :key="i"
                                class="bg-popover shadow-none"
                            />
                        </div>
                        <VirtualList
                            v-else
                            :items="pickerFiles"
                            :item-key="(file) => file.id"
                            :min-item-width="160"
                            :estimate-size="220"
                            :gap="12"
                            :loading="isPickerLoadingMore"
                            contained
                            class="pb-2"
                            @reach-end="loadMore"
                        >
                            <template #skeleton>
                                <FileCardSkeleton class="bg-popover shadow-none" />
                            </template>
                            <template #default="{ item }">
                                <FileCard
                                    :file="item"
                                    :selected="isPicked(item)"
                                    class="bg-popover shadow-none"
                                    selection-mode
                                    pickable
                                    @click="handleTogglePick(item)"
                                    @toggle="handleTogglePick(item)"
                                />
                            </template>
                        </VirtualList>
                    </div>

                    <UiDialogFooter class="items-center">
                        <TextBlock variant="muted" class="mr-auto">{{ pickerChanges }}</TextBlock>
                        <UiButton variant="outline" @click="pickerOpen = false">Cancel</UiButton>
                        <UiButton
                            :disabled="(!added.length && !removed.length) || isSaving"
                            @click="handleSavePicker"
                        >
                            <UiSpinner v-if="isSaving" />
                            Save
                        </UiButton>
                    </UiDialogFooter>
                </UiDialogContent>
            </UiDialog>
        </template>
    </div>
</template>

<script setup lang="ts">
import { ICircleAlert, IFiles, IFolderX } from '#components';

const route = useRoute();
const labels = useBreadcrumbLabels();
const { $toast } = useNuxtApp();
const { copyLink } = useFolderActions();

const formOpen = ref(false);
const pickerOpen = ref(false);
const pickerSearch = ref('');
const pickerFilter = ref<'all' | 'in' | 'out'>('all');
const added = ref<string[]>([]);
const removed = ref<string[]>([]);
const isSaving = ref(false);
const summary = ref<FileSummary>();

const pickerFilters = [
    { label: 'All files', value: 'all' },
    { label: 'In this folder', value: 'in' },
    { label: 'Not in this folder', value: 'out' },
];

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
    items: pickerFiles,
    isLoading: isPickerLoading,
    isLoadingMore: isPickerLoadingMore,
    isError: isPickerError,
    loadMore,
    refresh: refreshPicker,
} = usePaginatedList<FileData>(
    'files',
    '/api/files',
    () => ({
        q: pickerSearch.value.trim(),
        ...(pickerFilter.value === 'in' && { folder: folderId.value }),
        ...(pickerFilter.value === 'out' && { excludeFolder: folderId.value }),
    }),
    { limit: 40, enabled: pickerOpen },
);

const description = computed(
    () =>
        `${pluralize(summary.value?.count ?? folder.value!.fileCount, 'file')} · ${folder.value!.public ? 'Public' : 'Private'}`,
);

const pickerChanges = computed(() =>
    added.value.length || removed.value.length
        ? `${added.value.length} to add · ${removed.value.length} to remove`
        : 'No changes',
);

const isInFolder = (file: FileData) => file.folderId === folderId.value;

const isPicked = (file: FileData) =>
    isInFolder(file) ? !removed.value.includes(file.id) : added.value.includes(file.id);

const handleTogglePick = (file: FileData) => {
    const list = isInFolder(file) ? removed : added;

    list.value = list.value.includes(file.id)
        ? list.value.filter((id) => id !== file.id)
        : [...list.value, file.id];
};

const moveFiles = async (ids: string[], targetFolderId: string | null) => {
    for (const chunk of chunkIds(ids)) {
        await $fetch('/api/files/bulk-move', {
            method: 'POST',
            body: { files: chunk, folderId: targetFolderId },
        });
    }
};

const handleSavePicker = async () => {
    isSaving.value = true;

    try {
        await moveFiles(added.value, folderId.value);
        await moveFiles(removed.value, null);

        $toast.success('Folder updated');
        pickerOpen.value = false;
    } catch (error: any) {
        $toast.error(getErrorMessage(error));
    }

    isSaving.value = false;
};

useListHandle({
    resource: 'folders',
    refresh,
    map: (fn) => folder.value && (folder.value = fn(folder.value)),
    remove: (ids) => ids.includes(folderId.value) && navigateTo('/dashboard/folders'),
});

watch(folder, (value) => value && (labels.value = { ...labels.value, [value.id]: value.name }));

watch(pickerOpen, (value) => {
    if (!value) return;

    pickerSearch.value = '';
    pickerFilter.value = 'all';
    added.value = [];
    removed.value = [];
});

definePageMeta({
    layout: 'dashboard',
    middleware: 'file-uploader-only',
});
</script>
