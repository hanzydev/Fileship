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
            <UiInputGroup class="max-w-sm flex-1">
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
            v-else-if="!folders.length"
            :icon="search ? ISearchX : IFolder"
            :title="search ? $t('folders.noMatches') : $t('folders.empty')"
            :description="
                search ? $t('folders.noMatchesDescription') : $t('folders.emptyDescription')
            "
        >
            <UiButton v-if="!search" @click="openForm(null)">
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
                <UiContextMenu>
                    <UiContextMenuTrigger as-child>
                        <NuxtLink :to="`/dashboard/folders/${item.id}`">
                            <UiCard size="sm" class="group">
                                <UiCardContent>
                                    <div
                                        class="relative grid aspect-video grid-cols-2 grid-rows-2 gap-1 overflow-hidden rounded-2xl bg-muted"
                                    >
                                        <img
                                            v-for="(file, index) in item.covers"
                                            :key="file.id"
                                            :src="file.url"
                                            :alt="file.fileName"
                                            loading="lazy"
                                            :class="
                                                cn(
                                                    'size-full object-cover transition-transform duration-300',
                                                    item.public
                                                        ? 'group-hover:scale-105'
                                                        : 'scale-125 blur-2xl',
                                                    item.covers?.length === 1 &&
                                                        'col-span-2 row-span-2',
                                                    item.covers?.length === 2 && 'row-span-2',
                                                    item.covers?.length === 3 &&
                                                        index === 0 &&
                                                        'row-span-2',
                                                )
                                            "
                                        />
                                        <div
                                            v-if="item.covers?.length && !item.public"
                                            class="absolute inset-0 flex items-center justify-center"
                                        >
                                            <div
                                                class="flex size-10 items-center justify-center rounded-full bg-background/60 backdrop-blur-md"
                                            >
                                                <ILock class="size-4" />
                                            </div>
                                        </div>
                                        <div
                                            v-if="!item.covers?.length"
                                            class="col-span-2 row-span-2 flex items-center justify-center"
                                        >
                                            <IInbox
                                                v-if="item.isInbox"
                                                class="size-10 text-muted-foreground"
                                            />
                                            <IFolder v-else class="size-10 text-muted-foreground" />
                                        </div>
                                    </div>
                                </UiCardContent>
                                <UiCardFooter class="gap-2">
                                    <div class="flex min-w-0 flex-1 flex-col gap-0.5">
                                        <TextBlock variant="small" class="truncate">
                                            {{ item.name }}
                                        </TextBlock>
                                        <TextBlock variant="muted" class="text-xs">
                                            {{ $t('folders.fileCount', item.fileCount) }}
                                        </TextBlock>
                                    </div>
                                    <UiBadge v-if="item.isInbox" variant="secondary">
                                        {{ $t('folders.inbox') }}
                                    </UiBadge>
                                    <UiBadge v-if="item.public" variant="outline">
                                        <IGlobe />
                                        {{ $t('folders.public') }}
                                    </UiBadge>
                                    <UiDropdownMenu>
                                        <UiDropdownMenuTrigger as-child>
                                            <UiButton
                                                variant="ghost"
                                                size="icon-sm"
                                                @click.prevent.stop
                                            >
                                                <IEllipsis />
                                            </UiButton>
                                        </UiDropdownMenuTrigger>
                                        <UiDropdownMenuContent align="end" class="w-56">
                                            <FolderMenuItems
                                                :folder="item"
                                                type="dropdown"
                                                @edit="openForm(item)"
                                            />
                                        </UiDropdownMenuContent>
                                    </UiDropdownMenu>
                                </UiCardFooter>
                            </UiCard>
                        </NuxtLink>
                    </UiContextMenuTrigger>
                    <UiContextMenuContent class="w-56">
                        <FolderMenuItems :folder="item" type="context" @edit="openForm(item)" />
                    </UiContextMenuContent>
                </UiContextMenu>
            </template>
        </VirtualList>

        <FolderFormDialog v-model:open="formOpen" :folder="editingFolder" />
    </div>
</template>

<script setup lang="ts">
import { ICircleAlert, IFolder, ISearchX } from '#components';

import type { FolderData } from '~/types/folders';

const route = useRoute();
const router = useRouter();
const { t } = useI18n();

const search = ref('');
const sort = useQueryState('sort', 'createdAt:desc');

const sortOptions = computed(() => [
    { label: t('folders.sort.newest'), value: 'createdAt:desc' },
    { label: t('folders.sort.oldest'), value: 'createdAt:asc' },
    { label: t('folders.sort.nameAsc'), value: 'name:asc' },
    { label: t('folders.sort.nameDesc'), value: 'name:desc' },
    { label: t('folders.sort.mostFiles'), value: 'files:desc' },
]);
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
