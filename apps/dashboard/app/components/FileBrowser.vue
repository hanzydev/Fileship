<template>
    <div class="flex flex-col gap-4">
        <div class="flex flex-col gap-2 xl:flex-row xl:items-center">
            <SmartSearchInput
                ref="searchInput"
                v-model="searchInputValue"
                v-model:ai="aiSearch"
                class="xl:flex-1"
                placeholder="Search files..."
                :loading="!!query && isLoading"
                :ai-available="canUseAi"
            />

            <div class="flex flex-wrap items-center gap-2">
                <UiDropdownMenu v-if="!folderId">
                    <UiDropdownMenuTrigger as-child>
                        <UiButton variant="outline" class="max-w-52">
                            <IFolder />
                            <span class="truncate">{{ folderLabel }}</span>
                            <IChevronDown class="opacity-50" />
                        </UiButton>
                    </UiDropdownMenuTrigger>
                    <UiDropdownMenuContent align="start" class="w-56" @scroll="handleFolderScroll">
                        <UiDropdownMenuLabel>Folder</UiDropdownMenuLabel>
                        <UiDropdownMenuRadioGroup v-model="folderFilter">
                            <UiDropdownMenuRadioItem value="all">All files</UiDropdownMenuRadioItem>
                            <UiDropdownMenuRadioItem value="none">
                                Not in a folder
                            </UiDropdownMenuRadioItem>
                            <template v-if="folders.length">
                                <UiDropdownMenuSeparator />
                                <UiDropdownMenuRadioItem
                                    v-for="folder in folders"
                                    :key="folder.id"
                                    :value="folder.id"
                                >
                                    <span class="truncate">{{ folder.name }}</span>
                                </UiDropdownMenuRadioItem>
                            </template>
                        </UiDropdownMenuRadioGroup>
                    </UiDropdownMenuContent>
                </UiDropdownMenu>

                <UiDropdownMenu>
                    <UiDropdownMenuTrigger as-child>
                        <UiButton variant="outline">
                            <IListFilter />
                            Type
                            <UiBadge v-if="typeFilter.length" variant="secondary">
                                {{ typeFilter.length }}
                            </UiBadge>
                            <IChevronDown class="opacity-50" />
                        </UiButton>
                    </UiDropdownMenuTrigger>
                    <UiDropdownMenuContent align="start" class="w-52">
                        <UiDropdownMenuLabel>File type</UiDropdownMenuLabel>
                        <UiDropdownMenuCheckboxItem
                            v-for="(category, key) in FILE_CATEGORIES"
                            :key="key"
                            :model-value="typeFilter.includes(key)"
                            @update:model-value="toggleType(key)"
                            @select.prevent
                        >
                            <component :is="category.icon" />
                            {{ category.label }}
                            <UiDropdownMenuShortcut>
                                {{ summary?.categories[key] }}
                            </UiDropdownMenuShortcut>
                        </UiDropdownMenuCheckboxItem>
                        <template v-if="typeFilter.length">
                            <UiDropdownMenuSeparator />
                            <UiDropdownMenuItem @select="typeFilter = []">
                                <IX />
                                Clear filter
                            </UiDropdownMenuItem>
                        </template>
                    </UiDropdownMenuContent>
                </UiDropdownMenu>

                <UiDropdownMenu>
                    <UiDropdownMenuTrigger as-child>
                        <UiButton variant="outline">
                            <IArrowDownUp />
                            {{ sortOptions.find((option) => option.value === sortValue)?.label }}
                            <IChevronDown class="opacity-50" />
                        </UiButton>
                    </UiDropdownMenuTrigger>
                    <UiDropdownMenuContent align="start" class="w-44">
                        <UiDropdownMenuLabel>Sort by</UiDropdownMenuLabel>
                        <UiDropdownMenuRadioGroup v-model="sortValue">
                            <UiDropdownMenuRadioItem
                                v-for="option in sortOptions"
                                :key="option.value"
                                :value="option.value"
                            >
                                {{ option.label }}
                            </UiDropdownMenuRadioItem>
                        </UiDropdownMenuRadioGroup>
                    </UiDropdownMenuContent>
                </UiDropdownMenu>

                <UiToggleGroup
                    type="single"
                    variant="outline"
                    class="ml-auto"
                    :model-value="view"
                    @update:model-value="$event && (view = $event as never)"
                >
                    <UiToggleGroupItem value="grid" aria-label="Grid view">
                        <ILayoutGrid />
                    </UiToggleGroupItem>
                    <UiToggleGroupItem value="list" aria-label="List view">
                        <IList />
                    </UiToggleGroupItem>
                </UiToggleGroup>
            </div>
        </div>

        <div
            v-if="isLoading && view === 'grid'"
            class="grid grid-cols-[repeat(auto-fill,minmax(220px,1fr))] gap-4"
        >
            <FileCardSkeleton v-for="i in 8" :key="i" />
        </div>

        <EmptyState
            v-else-if="!isLoading && !files.length && !isFiltered"
            :icon="IFiles"
            :title="emptyTitle"
            description="Upload files with the button above, drop them anywhere or paste an image."
        >
            <UiButton @click="uploadSheetOpen = true">
                <IUpload />
                Upload files
            </UiButton>
        </EmptyState>

        <EmptyState
            v-else-if="!isLoading && !files.length"
            :icon="ISearchX"
            title="No matching files"
            description="Try a different search or clear your filters."
        >
            <UiButton variant="outline" @click="handleClearFilters">Clear filters</UiButton>
        </EmptyState>

        <VirtualList
            v-else-if="view === 'grid'"
            :items="files"
            :item-key="(f) => f.id"
            :min-item-width="220"
            :estimate-size="270"
            :gap="16"
            :loading="isLoadingMore"
            @reach-end="loadMore"
        >
            <template #default="{ item }">
                <FileCard
                    :file="item"
                    :selected="selected.includes(item.id)"
                    :selection-mode="!!selected.length"
                    @click="handleClick(item, $event)"
                    @toggle="handleToggle(item, $event)"
                    @view="openSheet(item)"
                    @edit="openSheet(item, 'edit')"
                    @move="moveTargets = [item]"
                />
            </template>
        </VirtualList>

        <DataTable
            v-else
            v-model:sort="tableSort"
            :items="files"
            :item-key="(f) => f.id"
            :columns="columns"
            :selected="selected"
            :loading="isLoading"
            :loading-more="isLoadingMore"
            @row-click="handleClick"
            @reach-end="loadMore"
        >
            <template #cell-select="{ item }">
                <UiCheckbox
                    :model-value="selected.includes(item.id)"
                    @click.stop="handleToggle(item, $event)"
                />
            </template>
            <template #cell-fileName="{ item }">
                <div class="flex min-w-0 items-center gap-3">
                    <div
                        class="flex size-9 shrink-0 items-center justify-center overflow-hidden rounded-lg bg-muted"
                    >
                        <img
                            v-if="item.mimeType.startsWith('image/')"
                            :src="item.directUrl"
                            loading="lazy"
                            class="size-full object-cover"
                        />
                        <component
                            :is="getFileIcon(item)"
                            v-else
                            class="size-4 text-muted-foreground"
                        />
                    </div>
                    <span class="truncate font-medium">{{ item.fileName }}</span>
                    <ILock v-if="item.password" class="size-3.5 shrink-0 text-muted-foreground" />
                </div>
            </template>
            <template #cell-type="{ item }">
                <UiBadge variant="secondary">
                    {{ FILE_CATEGORIES[getFileCategory(item)].label }}
                </UiBadge>
            </template>
            <template #cell-size="{ item }">
                <span class="block truncate">{{ item.size.formatted }}</span>
            </template>
            <template #cell-views="{ item }">{{ item.views.total }}</template>
            <template #cell-folder="{ item }">
                <span class="block truncate text-muted-foreground">
                    {{ item.folder?.name || '—' }}
                </span>
            </template>
            <template #cell-createdAt="{ item }">
                <span class="block truncate text-muted-foreground">
                    {{ dayjs(item.createdAt).fromNow() }}
                </span>
            </template>
            <template #cell-actions="{ item }">
                <div class="flex justify-end">
                    <UiDropdownMenu>
                        <UiDropdownMenuTrigger as-child>
                            <UiButton variant="ghost" size="icon-sm" @click.stop>
                                <IEllipsis />
                            </UiButton>
                        </UiDropdownMenuTrigger>
                        <UiDropdownMenuContent align="end" class="w-56">
                            <FileMenuItems
                                :file="item"
                                type="dropdown"
                                @view="openSheet(item)"
                                @edit="openSheet(item, 'edit')"
                                @move="moveTargets = [item]"
                            />
                        </UiDropdownMenuContent>
                    </UiDropdownMenu>
                </div>
            </template>
            <template #card-skeleton>
                <ListCardSkeleton media="size-12 rounded-xl" />
            </template>
            <template #card="{ item }">
                <UiCard size="sm" @click="handleClick(item, $event)">
                    <UiCardContent class="flex items-center gap-3">
                        <div
                            class="flex size-12 shrink-0 items-center justify-center overflow-hidden rounded-xl bg-muted"
                        >
                            <img
                                v-if="item.mimeType.startsWith('image/')"
                                :src="item.directUrl"
                                loading="lazy"
                                class="size-full object-cover"
                            />
                            <component
                                :is="getFileIcon(item)"
                                v-else
                                class="size-5 text-muted-foreground"
                            />
                        </div>
                        <div class="flex min-w-0 flex-1 flex-col">
                            <TextBlock variant="small" class="truncate">
                                {{ item.fileName }}
                            </TextBlock>
                            <TextBlock variant="muted" class="truncate text-xs">
                                {{ item.size.formatted }} · {{ dayjs(item.createdAt).fromNow() }}
                            </TextBlock>
                        </div>
                        <UiCheckbox
                            :model-value="selected.includes(item.id)"
                            @click.stop="handleToggle(item, $event)"
                        />
                    </UiCardContent>
                </UiCard>
            </template>
        </DataTable>

        <FileBulkBar v-model="selected" :items="files" :total="total" :query="listQuery" />

        <FileMoveDialog v-model="moveTargets" />

        <FileSheet
            v-model:file-id="sheetFileId"
            v-model:tab="sheetTab"
            :files="files"
            :has-more="hasMore"
            :load-more="loadMore"
        />
    </div>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';

import { IFiles, ISearchX } from '#components';

const { folderId, emptyTitle = 'No files yet' } = defineProps<{
    folderId?: string;
    emptyTitle?: string;
}>();

const summary = defineModel<FileSummary>('summary');

const currentUser = useAuthUser();
const runtimeConfig = useRuntimeConfig();
const uploadSheetOpen = useUploadSheet();

const query = useQueryState('q', '');
const typeQuery = useQueryState('type', '');
const sort = useQueryState('sort', '');
const view = useQueryState<'grid' | 'list'>('view', 'grid');
const folderFilter = useQueryState('folder', 'all');
const sheetFileId = useQueryState('file', '');

const searchInput = useTemplateRef('searchInput');
const searchInputValue = ref(query.value);
const aiSearch = ref(false);

const selected = ref<string[]>([]);
const lastSelected = ref<string | null>(null);

const sheetTab = ref('details');
const moveTargets = ref<Pick<FileData, 'id' | 'folderId'>[]>([]);

const canUseAi = computed(
    () => runtimeConfig.public.aiEnabled && currentUser.value!.aiSettings.enabled,
);

const sortOptions = computed(() => [
    ...(query.value ? [{ label: 'Relevance', value: 'relevance' }] : []),
    { label: 'Newest', value: 'createdAt:desc' },
    { label: 'Oldest', value: 'createdAt:asc' },
    { label: 'Name A–Z', value: 'fileName:asc' },
    { label: 'Name Z–A', value: 'fileName:desc' },
    { label: 'Largest', value: 'size:desc' },
    { label: 'Smallest', value: 'size:asc' },
    { label: 'Most viewed', value: 'views:desc' },
    { label: 'Least viewed', value: 'views:asc' },
]);

const columns = [
    { key: 'select', label: '', class: 'w-10' },
    { key: 'fileName', label: 'Name', sortable: true },
    { key: 'type', label: 'Type', class: 'hidden w-32 lg:table-cell' },
    { key: 'size', label: 'Size', sortable: true, descFirst: true, class: 'w-28' },
    {
        key: 'views',
        label: 'Views',
        sortable: true,
        descFirst: true,
        class: 'hidden w-24 lg:table-cell',
    },
    { key: 'folder', label: 'Folder', class: 'hidden w-44 xl:table-cell' },
    {
        key: 'createdAt',
        label: 'Uploaded',
        sortable: true,
        descFirst: true,
        class: 'hidden w-36 lg:table-cell',
    },
    { key: 'actions', label: '', class: 'w-16' },
];

const typeFilter = computed<FileCategory[]>({
    get: () => (typeQuery.value ? (typeQuery.value.split(',') as FileCategory[]) : []),
    set: (value) => (typeQuery.value = value.join(',')),
});

const folderLabel = computed(() =>
    folderFilter.value === 'all'
        ? 'All files'
        : folderFilter.value === 'none'
          ? 'Not in a folder'
          : folders.value.find((folder) => folder.id === folderFilter.value)?.name || 'Folder',
);

const sortValue = computed({
    get: () =>
        (sort.value === 'relevance' && !query.value ? '' : sort.value) ||
        (query.value ? 'relevance' : 'createdAt:desc'),
    set: (value) => (sort.value = value),
});

const tableSort = computed({
    get: () => {
        const [key, dir] = sortValue.value.split(':');
        return { key: key!, desc: dir === 'desc' };
    },
    set: (value) => (sort.value = `${value!.key}:${value!.desc ? 'desc' : 'asc'}`),
});

const isFiltered = computed(
    () => !!query.value || !!typeFilter.value.length || (!folderId && folderFilter.value !== 'all'),
);

const {
    items: files,
    total,
    summary: listSummary,
    query: listQuery,
    isLoading,
    isLoadingMore,
    hasMore,
    loadMore,
} = usePaginatedList<FileData, FileSummary>('files', '/api/files', () => ({
    q: query.value,
    mode: query.value && aiSearch.value ? 'vector' : undefined,
    sort: sortValue.value === 'relevance' ? undefined : sortValue.value,
    folder: folderId ?? folderFilter.value,
    type: typeFilter.value,
    summary: true,
}));

const folderList = folderId ? null : usePaginatedList<FolderData>('folders', '/api/folders');

const folders = computed(() => folderList?.items.value ?? []);

const handleFolderScroll = (event: Event) => {
    const target = event.target as HTMLElement;
    if (target.scrollHeight - target.scrollTop - target.clientHeight < 80) folderList?.loadMore();
};

const toggleType = (type: FileCategory) => {
    typeFilter.value = typeFilter.value.includes(type)
        ? typeFilter.value.filter((t) => t !== type)
        : [...typeFilter.value, type];
};

const openSheet = (file: FileData, tab = 'details') => {
    sheetTab.value = tab;
    sheetFileId.value = file.id;
};

const handleToggle = (file: FileData, event: MouseEvent) => {
    const ids = files.value.map((f) => f.id);
    const anchor = lastSelected.value ? ids.indexOf(lastSelected.value) : -1;

    if (event.shiftKey && anchor > -1) {
        const [start, end] = [anchor, ids.indexOf(file.id)].sort((a, b) => a - b);

        selected.value = [...new Set([...selected.value, ...ids.slice(start!, end! + 1)])];
    } else {
        selected.value = selected.value.includes(file.id)
            ? selected.value.filter((id) => id !== file.id)
            : [...selected.value, file.id];
    }

    lastSelected.value = file.id;
};

const handleClick = (file: FileData, event: MouseEvent) => {
    if (selected.value.length || event.shiftKey || event.ctrlKey || event.metaKey) {
        handleToggle(file, event);
    } else {
        openSheet(file);
    }
};

const handleClearFilters = () => {
    searchInputValue.value = '';
    typeFilter.value = [];
    folderFilter.value = 'all';
};

let searchTimer: ReturnType<typeof setTimeout> | undefined;

watch(searchInputValue, (value) => {
    clearTimeout(searchTimer);
    searchTimer = setTimeout(() => (query.value = value.trim()), 400);
});

watch(query, (value) => {
    if (value !== searchInputValue.value.trim()) searchInputValue.value = value;
});

onBeforeUnmount(() => clearTimeout(searchTimer));

watch(listSummary, (value) => (summary.value = value));

watch(
    () => JSON.stringify(listQuery.value),
    () => (selected.value = []),
);

onKeyStroke('/', (event) => {
    if (document.activeElement?.closest('input, textarea, [contenteditable="true"]')) return;

    event.preventDefault();
    searchInput.value!.focus();
});

onKeyStroke('Escape', () => {
    if (!sheetFileId.value && !moveTargets.value.length) selected.value = [];
});
</script>
