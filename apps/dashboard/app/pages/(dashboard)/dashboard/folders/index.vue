<template>
    <Head>
        <Title>Folders</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-4 p-4 pt-0">
        <PageHeader
            title="Folders"
            :description="isLoading ? 'Loading...' : pluralize(total, 'folder')"
        >
            <UiButton @click="openForm(null)">
                <IFolderPlus />
                New folder
            </UiButton>
        </PageHeader>

        <div class="flex flex-wrap items-center gap-2">
            <UiInputGroup class="max-w-sm flex-1">
                <UiInputGroupAddon>
                    <UiSpinner v-if="!!search && isFetching" />
                    <ISearch v-else />
                </UiInputGroupAddon>
                <UiInputGroupInput v-model="search" placeholder="Search folders..." />
            </UiInputGroup>

            <UiDropdownMenu>
                <UiDropdownMenuTrigger as-child>
                    <UiButton variant="outline">
                        <IArrowDownUp />
                        {{ sortOptions.find((option) => option.value === sort)?.label }}
                        <IChevronDown class="opacity-50" />
                    </UiButton>
                </UiDropdownMenuTrigger>
                <UiDropdownMenuContent align="end" class="w-56">
                    <UiDropdownMenuLabel>Sort by</UiDropdownMenuLabel>
                    <UiDropdownMenuRadioGroup v-model="sort">
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
        </div>

        <div v-if="isLoading" class="grid grid-cols-[repeat(auto-fill,minmax(240px,1fr))] gap-4">
            <FolderCardSkeleton v-for="i in 6" :key="i" />
        </div>

        <EmptyState
            v-else-if="isError"
            :icon="ICircleAlert"
            title="Couldn't load folders"
            description="Something went wrong. Check your connection and try again."
        >
            <UiButton variant="outline" @click="refresh">Try again</UiButton>
        </EmptyState>

        <EmptyState
            v-else-if="!folders.length"
            :icon="search ? ISearchX : IFolder"
            :title="search ? 'No matching folders' : 'No folders yet'"
            :description="
                search
                    ? 'Try a different name.'
                    : 'Create a folder to group files and share them with one link.'
            "
        >
            <UiButton v-if="!search" @click="openForm(null)">
                <IFolderPlus />
                New folder
            </UiButton>
        </EmptyState>

        <VirtualList
            v-else
            :items="folders"
            :item-key="(f) => f.id"
            :min-item-width="240"
            :estimate-size="230"
            :gap="16"
            :loading="isLoadingMore"
            @reach-end="loadMore"
        >
            <template #skeleton>
                <FolderCardSkeleton />
            </template>
            <template #default="{ item }">
                <FolderCard :folder="item" @edit="openForm(item)" />
            </template>
        </VirtualList>

        <FolderFormDialog v-model:open="formOpen" :folder="editingFolder" />
    </div>
</template>

<script setup lang="ts">
import { ICircleAlert, IFolder, ISearchX } from '#components';

const route = useRoute();
const router = useRouter();

const search = ref('');
const sort = useQueryState('sort', 'createdAt:desc');

const sortOptions = [
    { label: 'Newest', value: 'createdAt:desc' },
    { label: 'Oldest', value: 'createdAt:asc' },
    { label: 'Name A–Z', value: 'name:asc' },
    { label: 'Name Z–A', value: 'name:desc' },
    { label: 'Most files', value: 'files:desc' },
];
const formOpen = ref(false);
const editingFolder = ref<FolderData | null>(null);

const {
    items: folders,
    total,
    isLoading,
    isError,
    isFetching,
    isLoadingMore,
    loadMore,
    refresh,
} = usePaginatedList<FolderData>('folders', '/api/folders', () => ({
    q: search.value.trim(),
    sort: sort.value,
}));

const openForm = (folder: FolderData | null) => {
    editingFolder.value = folder;
    formOpen.value = true;
};

watch(
    () => route.query.new,
    (value) => {
        if (value !== '1') return;

        openForm(null);
        router.replace({ query: { ...route.query, new: undefined } });
    },
    { immediate: true },
);

definePageMeta({
    layout: 'dashboard',
    middleware: 'file-uploader-only',
});
</script>
