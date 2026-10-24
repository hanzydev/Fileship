<template>
    <Head>
        <Title>{{ $t('folders.title') }}</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-4 p-4 pt-0">
        <PageHeader
            :title="$t('folders.title')"
            :description="isLoading ? $t('common.loading') : $t('folders.count', total)"
        >
            <UiButton @click="openForm(null)">
                <IFolderPlus />
                {{ $t('folders.newFolder') }}
            </UiButton>
        </PageHeader>

        <div class="flex flex-wrap items-center gap-2">
            <UiTabs v-model="view">
                <UiTabsList>
                    <UiTabsTrigger value="mine">{{ $t('folders.tabs.mine') }}</UiTabsTrigger>
                    <UiTabsTrigger value="shared">{{ $t('folders.tabs.shared') }}</UiTabsTrigger>
                </UiTabsList>
            </UiTabs>

            <UiInputGroup class="max-w-sm min-w-48 flex-1">
                <UiInputGroupAddon>
                    <UiSpinner v-if="!!search && isFetching" />
                    <ISearch v-else />
                </UiInputGroupAddon>
                <UiInputGroupInput
                    v-model="search"
                    :placeholder="$t('folders.searchPlaceholder')"
                />
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
                    <UiDropdownMenuLabel>{{ $t('folders.sortBy') }}</UiDropdownMenuLabel>
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
            :title="$t('folders.loadFailed')"
            :description="$t('folders.connectionError')"
        >
            <UiButton variant="outline" @click="refresh">{{ $t('folders.tryAgain') }}</UiButton>
        </EmptyState>

        <EmptyState
            v-else-if="!folders.length && search"
            :icon="ISearchX"
            :title="$t('folders.noMatches')"
            :description="$t('folders.noMatchesDescription')"
        />

        <EmptyState
            v-else-if="!folders.length && isShared"
            :icon="IUsers"
            :title="$t('folders.sharedEmpty')"
            :description="$t('folders.sharedEmptyDescription')"
        />

        <EmptyState
            v-else-if="!folders.length"
            :icon="IFolder"
            :title="$t('folders.empty')"
            :description="$t('folders.emptyDescription')"
        >
            <UiButton @click="openForm(null)">
                <IFolderPlus />
                {{ $t('folders.newFolder') }}
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
                <FolderCard
                    :folder="item"
                    :show-parent="!!search.trim()"
                    :leavable="isShared"
                    @edit="openForm(item)"
                    @move="movingFolder = item"
                    @share="sharingFolder = item"
                />
            </template>
        </VirtualList>

        <FolderFormDialog v-model:open="formOpen" :folder="editingFolder" />
        <FolderMoveDialog v-model="movingFolder" />
        <FolderShareDialog v-model="sharingFolder" />
    </div>
</template>

<script setup lang="ts">
import { ICircleAlert, IFolder, ISearchX, IUsers } from '#components';

import type { FolderData } from '~/types/folders';

const route = useRoute();
const router = useRouter();
const { t } = useI18n();

const search = ref('');
const sort = useQueryState('sort', 'createdAt:desc');
const view = useQueryState('view', 'mine');

const isShared = computed(() => view.value === 'shared');

const sortOptions = computed(() => [
    { label: t('folders.sort.newest'), value: 'createdAt:desc' },
    { label: t('folders.sort.oldest'), value: 'createdAt:asc' },
    { label: t('folders.sort.nameAsc'), value: 'name:asc' },
    { label: t('folders.sort.nameDesc'), value: 'name:desc' },
    { label: t('folders.sort.mostFiles'), value: 'files:desc' },
]);
const formOpen = ref(false);
const editingFolder = ref<FolderData | null>(null);
const movingFolder = ref<FolderData | null>(null);
const sharingFolder = ref<FolderData | null>(null);

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
    parent: search.value.trim() || isShared.value ? undefined : 'root',
    shared: isShared.value || undefined,
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
