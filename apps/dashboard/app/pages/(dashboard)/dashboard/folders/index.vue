<template>
    <Head>
        <Title>Folders</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-6 p-4 pt-0">
        <PageHeader
            title="Folders"
            :description="isLoading ? 'Loading...' : pluralize(total, 'folder')"
        >
            <UiButton @click="openForm(null)">
                <IFolderPlus />
                New folder
            </UiButton>
        </PageHeader>

        <UiInputGroup class="max-w-sm">
            <UiInputGroupAddon>
                <ISearch />
            </UiInputGroupAddon>
            <UiInputGroupInput v-model="search" placeholder="Search folders..." />
        </UiInputGroup>

        <div v-if="isLoading" class="grid grid-cols-[repeat(auto-fill,minmax(240px,1fr))] gap-4">
            <UiSkeleton v-for="i in 6" :key="i" class="aspect-5/4 rounded-4xl" />
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
const formOpen = ref(false);
const editingFolder = ref<FolderData | null>(null);

const {
    items: folders,
    total,
    isLoading,
    isLoadingMore,
    loadMore,
} = usePaginatedList<FolderData>('folders', '/api/folders', () => ({ q: search.value.trim() }));

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
