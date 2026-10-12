<template>
    <Head>
        <Title>Trash</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-4 p-4 pt-0">
        <PageHeader
            title="Trash"
            :description="`Items are deleted forever after ${pluralize(retentionDays, 'day')}. They still count toward your storage until then.`"
        >
            <template v-if="selected.length">
                <UiButton variant="outline" @click="handleRestore(selected)">
                    <IRotateCcw />
                    Restore {{ selected.length }}
                </UiButton>
                <UiButton variant="outline" @click="handleDeleteForever(selected)">
                    <ITrash2 />
                    Delete forever
                </UiButton>
            </template>
            <UiButton v-else variant="outline" @click="handleEmpty">
                <ITrash2 />
                Empty trash
            </UiButton>
        </PageHeader>

        <div class="flex flex-wrap gap-2">
            <UiTabs v-model="kind">
                <UiTabsList>
                    <UiTabsTrigger v-for="tab in tabs" :key="tab.value" :value="tab.value">
                        {{ tab.label }}
                    </UiTabsTrigger>
                </UiTabsList>
            </UiTabs>
            <UiInputGroup class="max-w-sm min-w-48 flex-1">
                <UiInputGroupAddon>
                    <UiSpinner v-if="!!search && isLoading" />
                    <ISearch v-else />
                </UiInputGroupAddon>
                <UiInputGroupInput v-model="search" placeholder="Search trash..." />
            </UiInputGroup>
        </div>

        <EmptyState
            v-if="!isLoading && !isSwitching && !items.length"
            :icon="ITrash2"
            :title="search ? 'No matching items' : 'Nothing here'"
            :description="
                search
                    ? 'Try a different search.'
                    : `Deleted ${kind} show up here until they are deleted forever.`
            "
        />

        <DataTable
            v-else
            :key="kind"
            v-model:sort="sort"
            :items="items"
            :item-key="(item) => item.id"
            :columns="columns"
            :selected="selected"
            :card-estimate-size="74"
            :loading="isLoading || isSwitching"
            :loading-more="isLoadingMore"
            @reach-end="loadMore"
        >
            <template #cell-select="{ item }">
                <UiCheckbox
                    :model-value="selected.includes(item.id)"
                    @update:model-value="handleToggle(item.id)"
                />
            </template>
            <template #cell-name="{ item }">
                <div class="flex min-w-0 items-center gap-3">
                    <component :is="getIcon(item)" class="size-4 shrink-0 text-muted-foreground" />
                    <span class="truncate font-medium">{{ item.name }}</span>
                </div>
            </template>
            <template #cell-details="{ item }">
                <span class="block truncate text-muted-foreground">{{ getDetails(item) }}</span>
            </template>
            <template #cell-deletedAt="{ item }">
                <UiTooltip>
                    <UiTooltipTrigger as-child>
                        <span class="block truncate text-muted-foreground">
                            {{ dayjs(item.deletedAt).fromNow() }}
                        </span>
                    </UiTooltipTrigger>
                    <UiTooltipContent>
                        {{ dayjs(item.deletedAt).format('MMM D, YYYY HH:mm') }}
                    </UiTooltipContent>
                </UiTooltip>
            </template>
            <template #cell-purgeAt="{ item }">
                <span class="block truncate text-muted-foreground">
                    {{ dayjs(item.purgeAt).fromNow() }}
                </span>
            </template>
            <template #cell-actions="{ item }">
                <div class="flex justify-end">
                    <UiDropdownMenu>
                        <UiDropdownMenuTrigger as-child>
                            <UiButton variant="ghost" size="icon-sm">
                                <IEllipsis />
                            </UiButton>
                        </UiDropdownMenuTrigger>
                        <UiDropdownMenuContent align="end" class="w-56">
                            <UiDropdownMenuItem @select="handleRestore([item.id])">
                                <IRotateCcw />
                                Restore
                            </UiDropdownMenuItem>
                            <UiDropdownMenuSeparator />
                            <UiDropdownMenuItem
                                variant="destructive"
                                @select="handleDeleteForever([item.id])"
                            >
                                <ITrash2 />
                                Delete forever
                            </UiDropdownMenuItem>
                        </UiDropdownMenuContent>
                    </UiDropdownMenu>
                </div>
            </template>
            <template #card-skeleton>
                <ListCardSkeleton media="size-4" />
            </template>
            <template #card="{ item }">
                <UiCard size="sm">
                    <UiCardContent class="flex min-h-8 items-center gap-3">
                        <UiCheckbox
                            :model-value="selected.includes(item.id)"
                            @update:model-value="handleToggle(item.id)"
                        />
                        <component
                            :is="getIcon(item)"
                            class="size-4 shrink-0 text-muted-foreground"
                        />
                        <div class="flex min-w-0 flex-1 flex-col">
                            <TextBlock variant="small" class="truncate">{{ item.name }}</TextBlock>
                            <TextBlock variant="muted" class="truncate text-xs">
                                Deleted forever {{ dayjs(item.purgeAt).fromNow() }}
                            </TextBlock>
                        </div>
                        <UiButton variant="ghost" size="icon-sm" @click="handleRestore([item.id])">
                            <IRotateCcw />
                        </UiButton>
                        <UiButton
                            variant="ghost"
                            size="icon-sm"
                            @click="handleDeleteForever([item.id])"
                        >
                            <ITrash2 />
                        </UiButton>
                    </UiCardContent>
                </UiCard>
            </template>
        </DataTable>
    </div>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';

import { IBookmark, IFolder, ITrash2 } from '#components';

type TrashKind = 'files' | 'folders' | 'notes';

type TrashItem = {
    id: string;
    name: string;
    deletedAt: string;
    purgeAt: string;
    mimeType?: string;
    size?: { raw: string; formatted: string };
    fileCount?: number;
};

const { $toast } = useNuxtApp();
const { retentionDays, restore } = useTrash();

const kind = useQueryState<TrashKind>('kind', 'files');
const search = ref('');
const sort = ref({ key: 'deletedAt', desc: true });
const selected = ref<string[]>([]);

const tabs = [
    { label: 'Files', value: 'files' },
    { label: 'Folders', value: 'folders' },
    { label: 'Notes', value: 'notes' },
];

const columns = computed(() => [
    { key: 'select', label: '', class: 'w-10' },
    { key: 'name', label: 'Name', sortable: true },
    ...(kind.value === 'notes'
        ? []
        : [{ key: 'details', label: '', class: 'hidden w-32 lg:table-cell' }]),
    { key: 'deletedAt', label: 'Deleted', sortable: true, descFirst: true, class: 'w-36' },
    { key: 'purgeAt', label: 'Deleted forever', class: 'hidden w-36 md:table-cell' },
    { key: 'actions', label: '', class: 'w-16' },
]);

const { items, isLoading, isLoadingMore, isFresh, loadMore } = usePaginatedList<TrashItem>(
    'trash',
    '/api/trash',
    () => ({
        kind: kind.value,
        q: search.value.trim(),
        sort: `${sort.value.key}:${sort.value.desc ? 'desc' : 'asc'}`,
    }),
);

const isSwitching = ref(false);

watch(kind, () => {
    isSwitching.value = true;
    selected.value = [];
    search.value = '';
});

watch(isFresh, (value) => value && (isSwitching.value = false));

watch(items, (value) => {
    selected.value = selected.value.filter((id) => value.some((item) => item.id === id));
});

const getIcon = (item: TrashItem) =>
    item.mimeType
        ? getFileIcon({ mimeType: item.mimeType, fileName: item.name })
        : item.fileCount === undefined
          ? IBookmark
          : IFolder;

const getDetails = (item: TrashItem) =>
    item.size?.formatted ?? pluralize(item.fileCount ?? 0, 'file');

const handleToggle = (id: string) => {
    selected.value = selected.value.includes(id)
        ? selected.value.filter((selectedId) => selectedId !== id)
        : [...selected.value, id];
};

const handleRestore = async (ids: string[]) => {
    if (await restore({ [kind.value]: ids })) selected.value = [];
};

const handleDeleteForever = async (ids: string[]) => {
    await askConfirmation({
        title: `Delete ${ids.length > 1 ? pluralize(ids.length, 'item') : 'this item'} forever?`,
        description: 'This action cannot be undone.',
        confirmText: 'Delete forever',
        destructive: true,
        action: async () => {
            try {
                await $fetch('/api/trash/delete', {
                    method: 'POST',
                    body: { [kind.value]: ids },
                });

                selected.value = [];
                $toast.success('Deleted forever');
            } catch (error: any) {
                $toast.error(getErrorMessage(error));
                return false;
            }
        },
    });
};

const handleEmpty = async () => {
    await askConfirmation({
        title: 'Empty the trash?',
        description: 'Every file, folder and note in the trash will be deleted forever.',
        confirmText: 'Empty trash',
        destructive: true,
        action: async () => {
            try {
                const result = await withVerification((verificationData) =>
                    $fetch('/api/trash', { method: 'DELETE', body: { verificationData } }),
                );

                if (result) $toast.success('Trash emptied');
            } catch (error: any) {
                $toast.error(getErrorMessage(error));
            }
        },
    });
};

definePageMeta({
    layout: 'dashboard',
    middleware: 'user-only',
});
</script>
