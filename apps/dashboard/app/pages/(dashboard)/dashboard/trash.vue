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
                <UiButton
                    variant="outline"
                    :disabled="!!restoring.length"
                    @click="handleRestore(selected)"
                >
                    <UiSpinner v-if="restoring.length" />
                    <IRotateCcw v-else />
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
                    <UiSpinner v-if="!!search && isFetching" />
                    <ISearch v-else />
                </UiInputGroupAddon>
                <UiInputGroupInput v-model="search" placeholder="Search trash..." />
            </UiInputGroup>
        </div>

        <EmptyState
            v-if="isError"
            :icon="ICircleAlert"
            title="Couldn't load the trash"
            description="Something went wrong. Check your connection and try again."
        >
            <UiButton variant="outline" @click="refresh">Try again</UiButton>
        </EmptyState>

        <EmptyState
            v-else-if="!isLoading && !isSwitching && !items.length"
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
            @row-click="handleRowClick"
            @reach-end="loadMore"
        >
            <template #cell-select="{ item }">
                <UiCheckbox
                    :model-value="selected.includes(item.id)"
                    @click.stop
                    @update:model-value="handleToggle(item.id)"
                />
            </template>
            <template #cell-name="{ item }">
                <div class="flex min-w-0 items-center gap-3">
                    <div
                        v-if="item.mimeType"
                        class="relative flex size-9 shrink-0 items-center justify-center overflow-hidden rounded-lg bg-muted"
                    >
                        <FileThumbnail :file="item as any" />
                    </div>
                    <component
                        :is="getIcon(item)"
                        v-else
                        class="size-4 shrink-0 text-muted-foreground"
                    />
                    <span class="truncate font-medium">{{ item.name }}</span>
                </div>
            </template>
            <template #cell-type="{ item }">
                <UiBadge v-if="item.mimeType" variant="secondary">
                    {{ FILE_CATEGORIES[getFileCategory(item as any)].label }}
                </UiBadge>
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
                <div class="flex justify-end" @click.stop>
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
                <UiCard size="sm" @click="handleRowClick(item, $event)">
                    <UiCardContent class="flex min-h-8 items-center gap-3">
                        <UiCheckbox
                            :model-value="selected.includes(item.id)"
                            @click.stop
                            @update:model-value="handleToggle(item.id)"
                        />
                        <div
                            v-if="item.mimeType"
                            class="relative flex size-9 shrink-0 items-center justify-center overflow-hidden rounded-lg bg-muted"
                        >
                            <FileThumbnail :file="item as any" />
                        </div>
                        <component
                            :is="getIcon(item)"
                            v-else
                            class="size-4 shrink-0 text-muted-foreground"
                        />
                        <div class="flex min-w-0 flex-1 flex-col">
                            <TextBlock variant="small" class="truncate">{{ item.name }}</TextBlock>
                            <TextBlock variant="muted" class="truncate text-xs">
                                Deleted forever {{ dayjs(item.purgeAt).fromNow() }}
                            </TextBlock>
                        </div>
                        <UiButton
                            variant="ghost"
                            size="icon-sm"
                            :disabled="restoring.includes(item.id)"
                            @click.stop="handleRestore([item.id])"
                        >
                            <UiSpinner v-if="restoring.includes(item.id)" />
                            <IRotateCcw v-else />
                        </UiButton>
                        <UiButton
                            variant="ghost"
                            size="icon-sm"
                            @click.stop="handleDeleteForever([item.id])"
                        >
                            <ITrash2 />
                        </UiButton>
                    </UiCardContent>
                </UiCard>
            </template>
        </DataTable>

        <FileViewer
            v-model:file-id="viewerFileId"
            trash
            :files="trashedFiles"
            :total="total"
            :has-more="hasMore"
            :load-more="loadMore"
        />
    </div>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';

import { IBookmark, ICircleAlert, IFolder, ITrash2 } from '#components';

import type { FileData } from '~/types/files';

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
const viewerFileId = ref<string | null>(null);
const restoring = ref<string[]>([]);

const tabs = [
    { label: 'Files', value: 'files' },
    { label: 'Folders', value: 'folders' },
    { label: 'Notes', value: 'notes' },
];

const columns = computed(() => [
    { key: 'select', label: '', class: 'w-10' },
    { key: 'name', label: 'Name', sortable: true },
    ...(kind.value === 'files'
        ? [{ key: 'type', label: 'Type', class: 'hidden w-32 lg:table-cell' }]
        : []),
    ...(kind.value === 'notes'
        ? []
        : [
              {
                  key: 'details',
                  label: kind.value === 'files' ? 'Size' : 'Files',
                  class: 'hidden w-28 md:table-cell',
              },
          ]),
    { key: 'deletedAt', label: 'Deleted', sortable: true, descFirst: true, class: 'w-36' },
    { key: 'purgeAt', label: 'Deleted forever', class: 'hidden w-36 md:table-cell' },
    { key: 'actions', label: '', class: 'w-16' },
]);

const {
    items,
    total,
    hasMore,
    isLoading,
    isError,
    isFetching,
    isLoadingMore,
    isFresh,
    loadMore,
    refresh,
} = usePaginatedList<TrashItem>('trash', '/api/trash', () => ({
    kind: kind.value,
    q: search.value.trim(),
    sort: `${sort.value.key}:${sort.value.desc ? 'desc' : 'asc'}`,
}));

const isSwitching = ref(false);

watch(kind, () => {
    isSwitching.value = true;
    selected.value = [];
    search.value = '';
});

watch([isFresh, isError], ([fresh, error]) => (fresh || error) && (isSwitching.value = false));

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

const trashedFiles = computed(() =>
    kind.value === 'files' && !isSwitching.value ? (items.value as unknown as FileData[]) : [],
);

const handleRowClick = (item: TrashItem, event: MouseEvent) => {
    if (kind.value === 'files' && !selected.value.length && !event.shiftKey && !event.ctrlKey)
        viewerFileId.value = item.id;
    else handleToggle(item.id);
};

const handleToggle = (id: string) => {
    selected.value = selected.value.includes(id)
        ? selected.value.filter((selectedId) => selectedId !== id)
        : [...selected.value, id];
};

const handleRestore = async (ids: string[]) => {
    restoring.value = [...restoring.value, ...ids];
    if (await restore({ [kind.value]: ids })) selected.value = [];
    restoring.value = restoring.value.filter((id) => !ids.includes(id));
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
