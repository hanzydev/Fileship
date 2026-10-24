<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ $t('storage.title') }}</UiCardTitle>
            <UiCardDescription>{{ $t('storage.description') }}</UiCardDescription>
            <UiCardAction v-if="trashedSize">
                <UiButton variant="outline" @click="handleEmpty">
                    <ITrash2 />
                    <span class="hidden sm:inline">{{ $t('trash.emptyTrash') }}</span>
                </UiButton>
            </UiCardAction>
        </UiCardHeader>
        <UiCardContent class="flex flex-col gap-4">
            <div v-if="!storageUsage" class="flex flex-col gap-3">
                <UiSkeleton class="h-9 w-32 rounded-full" />
                <UiSkeleton class="h-1.5 w-full rounded-full" />
            </div>
            <template v-else>
                <div class="flex items-baseline justify-between gap-2">
                    <span class="text-3xl font-semibold tabular-nums">
                        {{ formatSize(Number(storageUsage.used)) }}
                    </span>
                    <span v-if="storageRatio !== null" class="text-muted-foreground tabular-nums">
                        {{ percentFormat.format(storageRatio) }}
                    </span>
                </div>
                <StorageMeter :used="storageUsage.used" :limit="storageUsage.limit" />
            </template>

            <dl class="grid grid-cols-[auto_1fr] gap-x-6 gap-y-2 text-sm">
                <dt class="text-muted-foreground">{{ $t('storage.limit') }}</dt>
                <dd class="text-right">
                    {{
                        storageUsage?.limit
                            ? formatSize(Number(storageUsage.limit))
                            : $t('storage.unlimited')
                    }}
                </dd>
                <dt class="text-muted-foreground">{{ $t('storage.inTrash') }}</dt>
                <dd class="text-right">{{ formatSize(trashedSize) }}</dd>
                <template v-if="stats">
                    <dt class="text-muted-foreground">{{ $t('storage.files') }}</dt>
                    <dd class="text-right">{{ numberFormat.format(stats.files.count) }}</dd>
                </template>
            </dl>
        </UiCardContent>
    </UiCard>

    <StatsDonut
        :title="$t('storage.byType')"
        :description="$t('storage.byTypeDescription')"
        :value-label="$t('home.size')"
        :is-loading="!stats && !statsFailed"
        :items="storageByType"
    />

    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ $t('storage.largest') }}</UiCardTitle>
            <UiCardDescription>{{ $t('storage.largestDescription') }}</UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <div v-if="isLoading" class="flex flex-col gap-2">
                <ListCardSkeleton v-for="i in 4" :key="i" media="size-10" />
            </div>

            <EmptyState
                v-else-if="isError"
                :icon="ICircleAlert"
                :title="$t('storage.largestFailed')"
                :description="$t('storage.largestFailedDescription')"
            >
                <UiButton variant="outline" @click="refresh">{{ $t('storage.tryAgain') }}</UiButton>
            </EmptyState>

            <EmptyState
                v-else-if="!files.length"
                :icon="IFiles"
                :title="$t('storage.noFiles')"
                :description="$t('storage.noFilesDescription')"
            />

            <VirtualList
                v-else
                :items="files"
                :item-key="(file) => file.id"
                :estimate-size="66"
                :loading="isLoadingMore"
                @reach-end="loadMore"
            >
                <template #skeleton>
                    <ListCardSkeleton media="size-10" />
                </template>
                <template #default="{ item }">
                    <NuxtLink :to="`/dashboard/files?file=${item.id}`" class="block">
                        <UiCard size="sm" class="transition-colors hover:bg-muted/50">
                            <UiCardContent class="flex items-center gap-3">
                                <div
                                    class="relative flex size-10 shrink-0 items-center justify-center overflow-hidden rounded-md bg-muted"
                                >
                                    <FileThumbnail :file="item" />
                                </div>
                                <div class="flex min-w-0 flex-1 flex-col">
                                    <TextBlock variant="small" class="truncate">
                                        {{ item.name }}
                                    </TextBlock>
                                    <TextBlock variant="muted" class="truncate">
                                        {{ date(item.createdAt).format('ll') }}
                                    </TextBlock>
                                </div>
                                <span class="shrink-0 text-sm tabular-nums">
                                    {{ item.size.formatted }}
                                </span>
                            </UiCardContent>
                        </UiCard>
                    </NuxtLink>
                </template>
            </VirtualList>
        </UiCardContent>
    </UiCard>

    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ $t('storage.duplicates') }}</UiCardTitle>
            <UiCardDescription>{{ $t('storage.duplicatesDescription') }}</UiCardDescription>
            <UiCardAction v-if="wastedSize">
                <UiBadge variant="secondary">
                    {{ $t('storage.canFree', { size: formatSize(wastedSize) }) }}
                </UiBadge>
            </UiCardAction>
        </UiCardHeader>
        <UiCardContent>
            <div v-if="isDuplicatesLoading" class="flex flex-col gap-2">
                <ListCardSkeleton v-for="i in 3" :key="i" media="size-10" />
            </div>

            <EmptyState
                v-else-if="isDuplicatesError"
                :icon="ICircleAlert"
                :title="$t('storage.duplicatesFailed')"
                :description="$t('storage.duplicatesFailedDescription')"
            >
                <UiButton variant="outline" @click="refreshDuplicates">
                    {{ $t('storage.tryAgain') }}
                </UiButton>
            </EmptyState>

            <EmptyState
                v-else-if="!duplicates.length"
                :icon="ICopyCheck"
                :title="$t('storage.noDuplicates')"
                :description="$t('storage.noDuplicatesDescription')"
            />

            <VirtualList
                v-else
                :items="duplicates"
                :item-key="(group) => group.id"
                :estimate-size="190"
                :loading="isLoadingMoreDuplicates"
                @reach-end="loadMoreDuplicates"
            >
                <template #skeleton>
                    <ListCardSkeleton media="size-10" />
                </template>
                <template #default="{ item }">
                    <UiCard size="sm">
                        <UiCardHeader>
                            <UiCardTitle>
                                {{
                                    $t('storage.copies', {
                                        count: item.files.length,
                                        size: formatSize(+item.size),
                                    })
                                }}
                            </UiCardTitle>
                            <UiCardDescription>
                                {{ $t('storage.canFree', { size: formatSize(+item.wasted) }) }}
                            </UiCardDescription>
                            <UiCardAction>
                                <UiButton
                                    variant="outline"
                                    size="sm"
                                    @click="handleKeepOldest(item)"
                                >
                                    <ITrash2 />
                                    {{ $t('storage.keepOldest') }}
                                </UiButton>
                            </UiCardAction>
                        </UiCardHeader>
                        <UiCardContent>
                            <UiItemGroup>
                                <UiItem
                                    v-for="file in item.files"
                                    :key="file.id"
                                    variant="muted"
                                    size="sm"
                                >
                                    <UiItemMedia variant="image" class="relative bg-muted">
                                        <FileThumbnail :file="file" />
                                    </UiItemMedia>
                                    <UiItemContent class="min-w-0">
                                        <UiItemTitle class="max-w-full">
                                            <span class="truncate">{{ file.name }}</span>
                                        </UiItemTitle>
                                        <UiItemDescription class="truncate">
                                            {{
                                                file.folder
                                                    ? $t('storage.fileInFolder', {
                                                          date: date(file.createdAt).format('ll'),
                                                          folder: file.folder.name,
                                                      })
                                                    : date(file.createdAt).format('ll')
                                            }}
                                        </UiItemDescription>
                                    </UiItemContent>
                                    <UiItemActions>
                                        <UiButton
                                            variant="ghost"
                                            size="icon-sm"
                                            :aria-label="$t('storage.openFile')"
                                            as-child
                                        >
                                            <NuxtLink :to="`/dashboard/files?file=${file.id}`">
                                                <IExternalLink />
                                            </NuxtLink>
                                        </UiButton>
                                        <UiButton
                                            variant="ghost"
                                            size="icon-sm"
                                            :aria-label="$t('storage.moveToTrash')"
                                            @click="remove([file])"
                                        >
                                            <ITrash2 />
                                        </UiButton>
                                    </UiItemActions>
                                </UiItem>
                            </UiItemGroup>
                        </UiCardContent>
                    </UiCard>
                </template>
            </VirtualList>
        </UiCardContent>
    </UiCard>
</template>

<script setup lang="ts">
import { ICircleAlert, ICopyCheck, IFiles } from '#components';

import type { DuplicateGroup, DuplicateSummary, FileData } from '~/types/files';

const { $toast } = useNuxtApp();
const { t, locale } = useI18n();
const date = useDate();
const stats = useStats();
const storageUsage = useStorageUsage();
const statsFailed = ref(false);
const { remove } = useFileActions();

const {
    items: files,
    isLoading,
    isLoadingMore,
    isError,
    loadMore,
    refresh,
} = usePaginatedList<FileData>('files', '/api/files', { sort: 'size:desc' });

const {
    items: duplicates,
    summary: duplicateSummary,
    isLoading: isDuplicatesLoading,
    isLoadingMore: isLoadingMoreDuplicates,
    isError: isDuplicatesError,
    loadMore: loadMoreDuplicates,
    refresh: refreshDuplicates,
} = usePaginatedList<DuplicateGroup, DuplicateSummary>('duplicates', '/api/files/duplicates');

const wastedSize = computed(() => Number(duplicateSummary.value?.wasted ?? 0));

const handleKeepOldest = (group: DuplicateGroup) => remove(group.files.slice(1));

const percentFormat = computed(
    () => new Intl.NumberFormat(locale.value, { style: 'percent', maximumFractionDigits: 0 }),
);
const numberFormat = computed(() => new Intl.NumberFormat(locale.value));

const storageRatio = computed(() =>
    storageUsage.value?.limit
        ? Number(storageUsage.value.used) / Number(storageUsage.value.limit)
        : null,
);

const trashedSize = computed(() => Number(stats.value?.storageUsed.trashed.raw ?? 0));

const storageByType = computed(() =>
    (stats.value?.byType ?? []).map((category) => ({
        label: t(FILE_CATEGORIES[category.type as FileCategory].label),
        value: +category.size,
        formatted: category.formatted,
    })),
);

const formatSize = (bytes: number) => formatStorageSize(bytes, locale.value);

const loadStats = () =>
    $fetch('/api/users/@me/stats')
        .then((data) => {
            stats.value = data;
            statsFailed.value = false;
        })
        .catch((error) => {
            statsFailed.value = true;
            $toast.error(getErrorMessage(error));
        });

const handleEmpty = async () => {
    await askConfirmation({
        title: t('trash.emptyTitle'),
        description: t('trash.emptyTrashDescription'),
        confirmText: t('trash.emptyTrash'),
        destructive: true,
        action: async () => {
            try {
                const result = await withVerification((verificationData) =>
                    $fetch('/api/trash', { method: 'DELETE', body: { verificationData } }),
                );

                if (!result) return;

                $toast.success(t('trash.emptied'));
                await loadStats();
            } catch (error: any) {
                $toast.error(getErrorMessage(error));
            }
        },
    });
};

onMounted(loadStats);

definePageMeta({
    layout: 'dashboard',
    middleware: 'file-uploader-only',
});
</script>
