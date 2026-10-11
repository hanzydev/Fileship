<template>
    <div class="flex flex-col gap-4">
        <div class="flex flex-col gap-2 xl:flex-row xl:items-center">
            <SmartSearchInput
                ref="searchInput"
                v-model="searchInputValue"
                v-model:ai="aiSearch"
                class="xl:flex-1"
                placeholder="Search files..."
                :loading="isSearching"
                :ai-available="canUseAi"
            />

            <div class="flex flex-wrap items-center gap-2">
                <UiSelect v-if="showFolderFilter" v-model="folderFilter">
                    <UiSelectTrigger class="w-44">
                        <IFolder />
                        <UiSelectValue />
                    </UiSelectTrigger>
                    <UiSelectContent position="popper">
                        <UiSelectItem value="all">All files</UiSelectItem>
                        <UiSelectItem value="none">Not in a folder</UiSelectItem>
                        <UiSelectSeparator v-if="folders.length" />
                        <UiSelectItem v-for="folder in folders" :key="folder.id" :value="folder.id">
                            {{ folder.name }}
                        </UiSelectItem>
                    </UiSelectContent>
                </UiSelect>

                <UiDropdownMenu>
                    <UiDropdownMenuTrigger as-child>
                        <UiButton variant="outline">
                            <IListFilter />
                            Type
                            <UiBadge v-if="typeFilter.length" variant="secondary">
                                {{ typeFilter.length }}
                            </UiBadge>
                        </UiButton>
                    </UiDropdownMenuTrigger>
                    <UiDropdownMenuContent align="end" class="w-48">
                        <UiDropdownMenuCheckboxItem
                            v-for="(category, key) in FILE_CATEGORIES"
                            :key="key"
                            :model-value="typeFilter.includes(key)"
                            @update:model-value="toggleType(key)"
                            @select.prevent
                        >
                            <component :is="category.icon" />
                            {{ category.label }}
                            <span class="ml-auto text-muted-foreground">{{ typeCounts[key] }}</span>
                        </UiDropdownMenuCheckboxItem>
                        <template v-if="typeFilter.length">
                            <UiDropdownMenuSeparator />
                            <UiDropdownMenuItem @select="typeFilter = []">
                                Clear filter
                            </UiDropdownMenuItem>
                        </template>
                    </UiDropdownMenuContent>
                </UiDropdownMenu>

                <UiSelect v-model="sort">
                    <UiSelectTrigger class="w-40">
                        <IArrowDownUp />
                        <UiSelectValue />
                    </UiSelectTrigger>
                    <UiSelectContent position="popper">
                        <UiSelectItem
                            v-for="option in sortOptions"
                            :key="option.value"
                            :value="option.value"
                        >
                            {{ option.label }}
                        </UiSelectItem>
                    </UiSelectContent>
                </UiSelect>

                <UiTabs
                    class="ml-auto"
                    :model-value="view"
                    @update:model-value="$event && (view = $event as never)"
                >
                    <UiTabsList>
                        <UiTabsTrigger value="grid" aria-label="Grid view">
                            <ILayoutGrid />
                        </UiTabsTrigger>
                        <UiTabsTrigger value="list" aria-label="List view">
                            <IList />
                        </UiTabsTrigger>
                    </UiTabsList>
                </UiTabs>
            </div>
        </div>

        <div v-if="isLoading" class="grid grid-cols-[repeat(auto-fill,minmax(220px,1fr))] gap-4">
            <UiSkeleton v-for="i in 8" :key="i" class="aspect-4/5 rounded-4xl" />
        </div>

        <EmptyState
            v-else-if="!files.length"
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
            v-else-if="!filtered.length"
            :icon="ISearchX"
            title="No matching files"
            description="Try a different search or clear your filters."
        >
            <UiButton variant="outline" @click="handleClearFilters">Clear filters</UiButton>
        </EmptyState>

        <VirtualList
            v-else-if="view === 'grid'"
            :items="filtered"
            :item-key="(f) => f.id"
            :min-item-width="220"
            :estimate-size="270"
            :gap="16"
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
            :items="filtered"
            :item-key="(f) => f.id"
            :columns="columns"
            :selected="selected"
            clickable
            @row-click="handleClick"
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
            <template #cell-size="{ item }">{{ item.size.formatted }}</template>
            <template #cell-views="{ item }">{{ item.views.total }}</template>
            <template #cell-folder="{ item }">
                <span class="text-muted-foreground">
                    {{ folders.find((f) => f.id === item.folderId)?.name || '—' }}
                </span>
            </template>
            <template #cell-createdAt="{ item }">
                <span class="text-muted-foreground">{{ dayjs(item.createdAt).fromNow() }}</span>
            </template>
            <template #cell-actions="{ item }">
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
            </template>
            <template #card="{ item }">
                <div
                    class="flex items-center gap-3 rounded-2xl p-2 hover:bg-muted/50"
                    @click="handleClick(item, $event)"
                >
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
                        <TextBlock variant="small" class="truncate">{{ item.fileName }}</TextBlock>
                        <TextBlock variant="muted" class="truncate text-xs">
                            {{ item.size.formatted }} · {{ dayjs(item.createdAt).fromNow() }}
                        </TextBlock>
                    </div>
                    <UiCheckbox
                        :model-value="selected.includes(item.id)"
                        @click.stop="handleToggle(item, $event)"
                    />
                </div>
            </template>
        </DataTable>

        <Transition
            enter-active-class="animate-in fade-in slide-in-from-bottom-4 duration-200"
            leave-active-class="animate-out fade-out slide-out-to-bottom-4 duration-200"
        >
            <div
                v-if="selected.length"
                class="fixed bottom-4 left-1/2 z-30 flex -translate-x-1/2 items-center gap-1 rounded-full border bg-popover p-1.5 pl-4 shadow-lg"
            >
                <TextBlock variant="small" class="mr-2 whitespace-nowrap">
                    {{ selected.length }} selected
                </TextBlock>
                <UiButton
                    variant="ghost"
                    size="sm"
                    class="hidden sm:flex"
                    @click="selected = isAllSelected ? [] : filtered.map((f) => f.id)"
                >
                    {{ isAllSelected ? 'Deselect all' : 'Select all' }}
                </UiButton>
                <UiButton variant="ghost" size="sm" @click="moveTargets = selectedFiles">
                    <IFolderInput />
                    <span class="hidden sm:inline">Move</span>
                </UiButton>
                <UiButton variant="ghost" size="sm" @click="download(selectedFiles)">
                    <IDownload />
                    <span class="hidden sm:inline">Download</span>
                </UiButton>
                <UiButton variant="ghost" size="sm" @click="handleBulkDelete">
                    <ITrash2 />
                    <span class="hidden sm:inline">Delete</span>
                </UiButton>
                <UiButton variant="ghost" size="icon-sm" @click="selected = []">
                    <IX />
                </UiButton>
            </div>
        </Transition>

        <UiDialog :open="!!moveTargets.length" @update:open="!$event && (moveTargets = [])">
            <UiDialogContent class="sm:max-w-md">
                <UiDialogHeader>
                    <UiDialogTitle>
                        Move {{ moveTargets.length > 1 ? `${moveTargets.length} files` : 'file' }}
                    </UiDialogTitle>
                    <UiDialogDescription>Choose a folder or create a new one.</UiDialogDescription>
                </UiDialogHeader>
                <FolderPicker v-model="moveFolderId" />
                <UiDialogFooter>
                    <UiButton variant="outline" @click="moveTargets = []">Cancel</UiButton>
                    <UiButton :disabled="isMoving" @click="handleMove">
                        <UiSpinner v-if="isMoving" />
                        Move
                    </UiButton>
                </UiDialogFooter>
            </UiDialogContent>
        </UiDialog>

        <FileSheet v-model:file-id="sheetFileId" v-model:tab="sheetTab" :files="filtered" />
    </div>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';

import { IFiles, ISearchX } from '#components';

const {
    files,
    searchEndpoint,
    showFolderFilter,
    isLoading,
    emptyTitle = 'No files yet',
} = defineProps<{
    files: FileData[];
    searchEndpoint: string;
    showFolderFilter?: boolean;
    isLoading?: boolean;
    emptyTitle?: string;
}>();

const folders = useFolders();
const currentUser = useAuthUser();
const runtimeConfig = useRuntimeConfig();
const uploadSheetOpen = useUploadSheet();
const { download, move, remove } = useFileActions();

const query = useQueryState('q', '');
const typeQuery = useQueryState('type', '');
const sort = useQueryState('sort', 'createdAt:desc');
const view = useQueryState<'grid' | 'list'>('view', 'grid');
const folderFilter = useQueryState('folder', 'all');
const sheetFileId = useQueryState('file', '');

const searchInput = useTemplateRef('searchInput');
const searchInputValue = ref(query.value);
const aiSearch = ref(false);
const isSearching = ref(false);
const searchResults = ref<string[] | null>(null);

const selected = ref<string[]>([]);
const lastSelected = ref<string | null>(null);

const sheetTab = ref('details');
const moveTargets = ref<FileData[]>([]);
const moveFolderId = ref<string | null>(null);
const isMoving = ref(false);

const canUseAi = computed(
    () => runtimeConfig.public.aiEnabled && currentUser.value!.aiSettings.enabled,
);

const sortOptions = [
    { label: 'Newest', value: 'createdAt:desc' },
    { label: 'Oldest', value: 'createdAt:asc' },
    { label: 'Name', value: 'fileName:asc' },
    { label: 'Largest', value: 'size:desc' },
    { label: 'Smallest', value: 'size:asc' },
    { label: 'Most viewed', value: 'views:desc' },
];

const columns = [
    { key: 'select', label: '', class: 'w-10' },
    { key: 'fileName', label: 'Name', sortable: true, class: 'max-w-0 w-full' },
    { key: 'type', label: 'Type', class: 'hidden lg:table-cell' },
    { key: 'size', label: 'Size', sortable: true },
    { key: 'views', label: 'Views', sortable: true, class: 'hidden md:table-cell' },
    { key: 'folder', label: 'Folder', class: 'hidden xl:table-cell' },
    { key: 'createdAt', label: 'Uploaded', sortable: true, class: 'hidden md:table-cell' },
    { key: 'actions', label: '', class: 'w-12' },
];

const typeFilter = computed<FileCategory[]>({
    get: () => (typeQuery.value ? (typeQuery.value.split(',') as FileCategory[]) : []),
    set: (value) => (typeQuery.value = value.join(',')),
});

const tableSort = computed({
    get: () => {
        const [key, dir] = sort.value.split(':');
        return { key: key!, desc: dir === 'desc' };
    },
    set: (value) => (sort.value = `${value!.key}:${value!.desc ? 'desc' : 'asc'}`),
});

const scoped = computed(() =>
    files.filter((f) => {
        if (folderFilter.value === 'all') return true;
        if (folderFilter.value === 'none') return !f.folderId;
        return f.folderId === folderFilter.value;
    }),
);

const typeCounts = computed(() => {
    const counts = Object.fromEntries(Object.keys(FILE_CATEGORIES).map((key) => [key, 0]));
    scoped.value.forEach((f) => counts[getFileCategory(f)]!++);
    return counts as Record<FileCategory, number>;
});

const filtered = computed(() => {
    const [key, dir] = sort.value.split(':') as [string, 'asc' | 'desc'];
    const results = searchResults.value && new Set(searchResults.value);

    const values = {
        createdAt: (f: FileData) => +f.createdAt,
        fileName: (f: FileData) => f.fileName.toLowerCase(),
        size: (f: FileData) => +f.size.raw,
        views: (f: FileData) => f.views.total,
    };

    const getValue = values[key as keyof typeof values] || values.createdAt;

    return scoped.value
        .filter(
            (f) =>
                (!typeFilter.value.length || typeFilter.value.includes(getFileCategory(f))) &&
                (!results || results.has(f.id)),
        )
        .sort((a, b) => {
            const result = getValue(a) > getValue(b) ? 1 : getValue(a) < getValue(b) ? -1 : 0;
            return dir === 'desc' ? -result : result;
        });
});

const isAllSelected = computed(() => selected.value.length === filtered.value.length);

const selectedFiles = computed(() => files.filter((f) => selected.value.includes(f.id)));

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
    if (event.shiftKey && lastSelected.value) {
        const ids = filtered.value.map((f) => f.id);
        const [start, end] = [ids.indexOf(lastSelected.value), ids.indexOf(file.id)].sort(
            (a, b) => a - b,
        );

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

const handleMove = async () => {
    isMoving.value = true;

    if (await move(moveTargets.value, moveFolderId.value)) {
        moveTargets.value = [];
        selected.value = [];
    }

    isMoving.value = false;
};

const handleBulkDelete = async () => {
    if (await remove(selectedFiles.value)) selected.value = [];
};

const handleSearch = async () => {
    if (!query.value) {
        searchResults.value = null;
        return;
    }

    isSearching.value = true;

    try {
        searchResults.value = await $fetch<string[]>(searchEndpoint, {
            method: 'POST',
            body: { query: query.value, mode: aiSearch.value ? 'vector' : 'fulltext' },
        });
    } catch {
        searchResults.value = [];
    }

    isSearching.value = false;
};

watchDebounced(searchInputValue, (value) => (query.value = value.trim()), { debounce: 400 });
watch([query, aiSearch], handleSearch, { immediate: true });

watch(moveTargets, (value) => {
    if (value.length) moveFolderId.value = value.length === 1 ? value[0]!.folderId : null;
});

watch(
    () => files.length,
    () => (selected.value = selected.value.filter((id) => files.some((f) => f.id === id))),
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
