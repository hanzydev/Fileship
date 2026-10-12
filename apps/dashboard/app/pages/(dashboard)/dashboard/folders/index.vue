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
                    <UiSpinner v-if="!!search && isLoading" />
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
            <UiCard v-for="i in 6" :key="i" size="sm">
                <UiCardContent>
                    <UiSkeleton class="aspect-video" />
                </UiCardContent>
                <UiCardFooter class="gap-2">
                    <div class="flex min-w-0 flex-1 flex-col gap-0.5">
                        <TextBlock variant="small" class="w-full">
                            <UiSkeleton class="h-lh w-2/3 rounded-full" />
                        </TextBlock>
                        <TextBlock variant="muted" class="w-full text-xs">
                            <UiSkeleton class="h-lh w-1/3 rounded-full" />
                        </TextBlock>
                    </div>
                    <UiSkeleton class="size-8 rounded-full" />
                </UiCardFooter>
            </UiCard>
        </div>

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
            <template #default="{ item }">
                <FolderCard :folder="item" @edit="openForm(item)" />
            </template>
        </VirtualList>

        <FolderFormDialog v-model:open="formOpen" :folder="editingFolder" />
    </div>
</template>

<script setup lang="ts">
import { IFolder, ISearchX } from '#components';

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
    isLoadingMore,
    loadMore,
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
