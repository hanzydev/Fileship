<template>
    <Head>
        <Title>{{ folder?.name || $t('folders.folder') }}</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-4 p-4 pt-0">
        <EmptyState
            v-if="!folder && !isLoading"
            :icon="IFolderX"
            :title="$t('folders.detail.notFound')"
            :description="$t('folders.detail.notFoundDescription')"
        >
            <UiButton variant="outline" as-child>
                <NuxtLink to="/dashboard/folders">{{ $t('folders.detail.back') }}</NuxtLink>
            </UiButton>
        </EmptyState>

        <template v-else-if="folder">
            <PageHeader :title="folder.name" :description="description">
                <UiButton
                    v-if="isEditableItem(folder)"
                    variant="outline"
                    @click="newFolderOpen = true"
                >
                    <IFolderPlus />
                    {{ $t('folders.newFolder') }}
                </UiButton>
                <UiButton v-if="isOwnedItem(folder)" variant="outline" @click="pickerOpen = true">
                    <IFilePlus />
                    {{ $t('folders.detail.manageFiles') }}
                </UiButton>
                <UiButton v-if="folder.public" variant="outline" @click="copyLink(folder)">
                    <ILink />
                    {{ $t('folders.detail.copyLink') }}
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
                            :leavable="!isOwnedItem(folder) && !folder.ancestors!.length"
                            @edit="openForm(folder)"
                            @move="movingFolder = folder"
                            @share="sharingFolder = folder"
                        />
                    </UiDropdownMenuContent>
                </UiDropdownMenu>
            </PageHeader>

            <UiAlert v-if="folder.isInbox">
                <IInbox />
                <UiAlertTitle>{{ $t('folders.detail.inboxTitle') }}</UiAlertTitle>
                <UiAlertDescription>
                    {{ $t('folders.detail.inboxDescription') }}
                    <NuxtLink to="/dashboard/settings/inbox" class="underline underline-offset-4">
                        {{ $t('folders.detail.inboxSettings') }}
                    </NuxtLink>
                </UiAlertDescription>
            </UiAlert>

            <section v-if="subfolders.length" class="flex flex-col gap-3">
                <TextBlock variant="large" custom-tag="h2">
                    {{ $t('folders.detail.subfolders') }}
                </TextBlock>
                <VirtualList
                    :items="subfolders"
                    :item-key="(f) => f.id"
                    :min-item-width="240"
                    :estimate-size="230"
                    :gap="16"
                    :loading="isLoadingMoreSubfolders"
                    @reach-end="loadMoreSubfolders"
                >
                    <template #skeleton>
                        <FolderCardSkeleton />
                    </template>
                    <template #default="{ item }">
                        <FolderCard
                            :folder="item"
                            @edit="openForm(item)"
                            @move="movingFolder = item"
                            @share="sharingFolder = item"
                        />
                    </template>
                </VirtualList>
            </section>

            <FileBrowser
                v-model:summary="summary"
                :folder-id="folder.id"
                :empty-title="$t('folders.detail.empty')"
            />

            <FolderFormDialog v-model:open="formOpen" :folder="editingFolder" />
            <FolderFormDialog v-model:open="newFolderOpen" :parent="folder" />
            <FolderMoveDialog v-model="movingFolder" />
            <FolderShareDialog v-model="sharingFolder" />

            <UiDialog v-model:open="pickerOpen">
                <UiDialogContent class="flex h-[min(85svh,52rem)] flex-col sm:max-w-4xl">
                    <UiDialogHeader>
                        <UiDialogTitle>
                            {{ $t('folders.picker.title', { name: folder.name }) }}
                        </UiDialogTitle>
                        <UiDialogDescription>
                            {{ $t('folders.picker.description') }}
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
                                :placeholder="$t('folders.picker.searchPlaceholder')"
                            />
                        </UiInputGroup>
                    </div>

                    <div class="-mx-2 min-h-0 flex-1 overflow-y-auto px-2 pt-1">
                        <EmptyState
                            v-if="isPickerError"
                            :icon="ICircleAlert"
                            :title="$t('folders.picker.loadFailed')"
                            :description="$t('folders.connectionError')"
                        >
                            <UiButton variant="outline" @click="refreshPicker">
                                {{ $t('folders.tryAgain') }}
                            </UiButton>
                        </EmptyState>
                        <EmptyState
                            v-else-if="!isPickerLoading && !pickerFiles.length"
                            :icon="IFiles"
                            :title="$t('folders.picker.empty')"
                            :description="$t('folders.picker.emptyDescription')"
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
                        <UiButton variant="outline" @click="pickerOpen = false">
                            {{ $t('common.cancel') }}
                        </UiButton>
                        <UiButton
                            :disabled="(!added.length && !removed.length) || isSaving"
                            @click="handleSavePicker"
                        >
                            <UiSpinner v-if="isSaving" />
                            {{ $t('folders.picker.save') }}
                        </UiButton>
                    </UiDialogFooter>
                </UiDialogContent>
            </UiDialog>
        </template>
    </div>
</template>

<script setup lang="ts">
import { ICircleAlert, IFiles, IFolderX } from '#components';

import type { FileData, FileSummary } from '~/types/files';
import type { FolderData } from '~/types/folders';

const route = useRoute();
const labels = useBreadcrumbLabels();
const trail = useBreadcrumbTrail();
const { $toast } = useNuxtApp();
const { t } = useI18n();
const { copyLink } = useFolderActions();

const formOpen = ref(false);
const newFolderOpen = ref(false);
const editingFolder = ref<FolderData | null>(null);
const movingFolder = ref<FolderData | null>(null);
const sharingFolder = ref<FolderData | null>(null);
const pickerOpen = ref(false);
const pickerSearch = ref('');
const pickerFilter = ref<'all' | 'in' | 'out'>('all');
const added = ref<string[]>([]);
const removed = ref<string[]>([]);
const isSaving = ref(false);
const summary = ref<FileSummary>();

const pickerFilters = computed(() => [
    { label: t('folders.picker.all'), value: 'all' },
    { label: t('folders.picker.in'), value: 'in' },
    { label: t('folders.picker.out'), value: 'out' },
]);

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
    items: subfolders,
    isLoadingMore: isLoadingMoreSubfolders,
    loadMore: loadMoreSubfolders,
} = usePaginatedList<FolderData>('folders', () => `/api/folders/${folderId.value}/folders`);

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

const description = computed(() => {
    const count = summary.value?.count ?? folder.value!.fileCount;

    if (!isOwnedItem(folder.value!))
        return t(
            'folders.detail.sharedDescription',
            { n: count, username: folder.value!.owner!.username },
            count,
        );

    return t(
        folder.value!.public
            ? 'folders.detail.publicDescription'
            : 'folders.detail.privateDescription',
        count,
    );
});

const pickerChanges = computed(() =>
    added.value.length || removed.value.length
        ? t('folders.picker.changes', { added: added.value.length, removed: removed.value.length })
        : t('folders.picker.noChanges'),
);

const openForm = (target: FolderData) => {
    editingFolder.value = target;
    formOpen.value = true;
};

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

        $toast.success(t('folders.detail.updated'));
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

watch(folder, (value) => {
    if (!value) return;

    labels.value = { ...labels.value, [value.id]: value.name };
    trail.value = {
        path: route.path,
        items: value.ancestors!.map((ancestor) => ({
            to: `/dashboard/folders/${ancestor.id}`,
            label: ancestor.name,
        })),
    };
});

watch(
    () => folder.value?.parentId,
    (value, previous) => previous !== undefined && value !== previous && refresh(),
);

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
