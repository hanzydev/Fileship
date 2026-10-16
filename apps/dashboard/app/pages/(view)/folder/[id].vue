<template>
    <div class="flex min-h-svh flex-col">
        <Head>
            <Title>{{ data!.name }}</Title>
        </Head>

        <ViewHeader :title="data!.name" :subtitle="subtitle" content-class="max-w-7xl">
            <UiTooltip :disabled="!data!.lockedCount">
                <UiTooltipTrigger as-child>
                    <UiButton
                        size="sm"
                        :disabled="!data!.fileCount || isDownloading"
                        @click="handleDownload"
                    >
                        <UiSpinner v-if="isDownloading" />
                        <IDownload v-else />
                        <span class="hidden sm:inline">
                            {{ $t('publicPages.folder.downloadAll') }}
                        </span>
                    </UiButton>
                </UiTooltipTrigger>
                <UiTooltipContent>
                    {{ $t('publicPages.folder.lockedNotIncluded') }}
                </UiTooltipContent>
            </UiTooltip>
        </ViewHeader>

        <main class="mx-auto flex w-full max-w-7xl flex-1 flex-col gap-4 p-4">
            <div class="flex flex-col gap-2 lg:flex-row lg:items-center">
                <SmartSearchInput
                    v-model="search"
                    v-model:ai="aiSearch"
                    class="lg:flex-1"
                    :placeholder="$t('publicPages.folder.searchPlaceholder')"
                    :loading="!!search && isFetching"
                    :ai-available="!!data!.authorId"
                />
                <div class="flex flex-wrap items-center gap-2">
                    <UiDropdownMenu>
                        <UiDropdownMenuTrigger as-child>
                            <UiButton variant="outline">
                                <IListFilter />
                                {{ $t('publicPages.folder.type') }}
                                <UiBadge v-if="typeFilter.length" variant="secondary">
                                    {{ typeFilter.length }}
                                </UiBadge>
                                <IChevronDown class="opacity-50" />
                            </UiButton>
                        </UiDropdownMenuTrigger>
                        <UiDropdownMenuContent align="start" class="w-56">
                            <UiDropdownMenuLabel>
                                {{ $t('publicPages.folder.fileType') }}
                            </UiDropdownMenuLabel>
                            <UiDropdownMenuCheckboxItem
                                v-for="(category, key) in FILE_CATEGORIES"
                                :key="key"
                                :model-value="typeFilter.includes(key)"
                                @update:model-value="toggleType(key)"
                                @select.prevent
                            >
                                <component :is="category.icon" />
                                {{ $t(category.label) }}
                            </UiDropdownMenuCheckboxItem>
                            <template v-if="typeFilter.length">
                                <UiDropdownMenuSeparator />
                                <UiDropdownMenuItem @select="typeFilter = []">
                                    <IX />
                                    {{ $t('publicPages.folder.clearFilter') }}
                                </UiDropdownMenuItem>
                            </template>
                        </UiDropdownMenuContent>
                    </UiDropdownMenu>

                    <UiDropdownMenu>
                        <UiDropdownMenuTrigger as-child>
                            <UiButton variant="outline">
                                <IArrowDownUp />
                                {{
                                    sortOptions.find((option) => option.value === sortValue)?.label
                                }}
                                <IChevronDown class="opacity-50" />
                            </UiButton>
                        </UiDropdownMenuTrigger>
                        <UiDropdownMenuContent align="start" class="w-56">
                            <UiDropdownMenuLabel>
                                {{ $t('publicPages.folder.sortBy') }}
                            </UiDropdownMenuLabel>
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
                </div>
            </div>

            <div
                v-if="isLoading"
                class="grid grid-cols-[repeat(auto-fill,minmax(220px,1fr))] gap-4"
            >
                <FileCardSkeleton v-for="i in 8" :key="i" />
            </div>

            <EmptyState
                v-else-if="isError"
                :icon="ICircleAlert"
                :title="$t('publicPages.folder.loadFailed')"
                :description="$t('publicPages.folder.loadFailedDescription')"
            >
                <UiButton variant="outline" @click="refresh">
                    {{ $t('publicPages.folder.tryAgain') }}
                </UiButton>
            </EmptyState>

            <EmptyState
                v-else-if="!files.length && !isFiltered"
                :icon="IFolderOpen"
                :title="$t('publicPages.folder.empty')"
            />

            <EmptyState
                v-else-if="!files.length"
                :icon="ISearchX"
                :title="$t('publicPages.folder.noMatches')"
                :description="$t('publicPages.folder.noMatchesDescription')"
            >
                <UiButton variant="outline" @click="handleClearFilters">
                    {{ $t('publicPages.folder.clearFilters') }}
                </UiButton>
            </EmptyState>

            <VirtualList
                v-else
                :items="visibleFiles"
                :item-key="(f) => f.id"
                :min-item-width="220"
                :estimate-size="260"
                :gap="16"
                :loading="isLoadingMore"
                @reach-end="loadMore"
            >
                <template #skeleton>
                    <FileCardSkeleton />
                </template>
                <template #default="{ item }">
                    <UiCard
                        size="sm"
                        class="group"
                        tabindex="0"
                        @click="handleOpen(item)"
                        @keydown.enter.self="handleOpen(item)"
                    >
                        <UiCardContent>
                            <div class="relative aspect-4/3 overflow-hidden rounded-2xl bg-muted">
                                <div
                                    v-if="item.isLocked"
                                    class="flex size-full flex-col items-center justify-center gap-2 text-muted-foreground"
                                >
                                    <ILock class="size-8" />
                                    <TextBlock variant="muted" class="text-xs">
                                        {{ $t('publicPages.folder.passwordProtected') }}
                                    </TextBlock>
                                </div>
                                <img
                                    v-else-if="
                                        item.mimeType.startsWith('image/') || item.thumbnailUrl
                                    "
                                    :src="withPassword(item, item.thumbnailUrl || item.directUrl)"
                                    :alt="item.name"
                                    loading="lazy"
                                    class="size-full object-cover transition-transform duration-300 group-hover:scale-105"
                                />
                                <div v-else class="flex size-full items-center justify-center">
                                    <component
                                        :is="getFileIcon(item)"
                                        class="size-10 text-muted-foreground"
                                    />
                                </div>
                                <div
                                    v-if="
                                        !item.isLocked &&
                                        item.mimeType.startsWith('video/') &&
                                        item.thumbnailUrl
                                    "
                                    class="absolute inset-0 flex items-center justify-center"
                                >
                                    <IPlay class="size-8 fill-white text-white drop-shadow" />
                                </div>
                            </div>
                        </UiCardContent>
                        <UiCardFooter class="flex-col items-start gap-0.5">
                            <div class="flex w-full items-center gap-1.5">
                                <ILock
                                    v-if="unlocked[item.id] || item.isLocked"
                                    class="size-3.5 shrink-0 text-muted-foreground"
                                />
                                <TextBlock variant="small" class="truncate">
                                    {{ item.name }}
                                </TextBlock>
                            </div>
                            <TextBlock variant="muted" class="truncate text-xs">
                                {{ item.size.formatted }}
                            </TextBlock>
                        </UiCardFooter>
                    </UiCard>
                </template>
            </VirtualList>
        </main>

        <UiDialog :open="!!current" @update:open="!$event && (current = null)">
            <UiDialogContent
                v-if="shownFile"
                class="flex h-[90svh] max-w-[95vw] flex-col gap-4 sm:max-w-[95vw]"
            >
                <UiDialogHeader>
                    <UiDialogTitle class="truncate pr-10">{{ shownFile.name }}</UiDialogTitle>
                    <UiDialogDescription>{{ shownFile.size.formatted }}</UiDialogDescription>
                </UiDialogHeader>
                <div class="relative flex min-h-0 flex-1 items-center justify-center">
                    <img
                        v-if="shownFile.mimeType.startsWith('image/')"
                        :src="withPassword(shownFile, shownFile.directUrl)"
                        :alt="shownFile.name"
                        class="max-h-full max-w-full rounded-2xl object-contain"
                    />
                    <video
                        v-else-if="shownFile.mimeType.startsWith('video/')"
                        :key="shownFile.id"
                        :src="withPassword(shownFile, shownFile.directUrl)"
                        :poster="
                            shownFile.thumbnailUrl
                                ? withPassword(shownFile, shownFile.thumbnailUrl)
                                : undefined
                        "
                        controls
                        autoplay
                        class="max-h-full max-w-full rounded-2xl"
                    />
                    <audio
                        v-else
                        :key="shownFile.id"
                        :src="withPassword(shownFile, shownFile.directUrl)"
                        controls
                        autoplay
                    />

                    <div class="absolute top-1/2 left-0 -translate-y-1/2">
                        <UiButton
                            variant="outline"
                            size="icon-sm"
                            :disabled="!prev"
                            @click="current = prev!"
                        >
                            <IChevronLeft />
                        </UiButton>
                    </div>
                    <div class="absolute top-1/2 right-0 -translate-y-1/2">
                        <UiButton
                            variant="outline"
                            size="icon-sm"
                            :disabled="!next"
                            @click="current = next!"
                        >
                            <IChevronRight />
                        </UiButton>
                    </div>
                </div>
                <UiDialogFooter>
                    <UiButton variant="outline" as-child>
                        <a :href="withPassword(shownFile, shownFile.embedUrl)" target="_blank">
                            <IExternalLink />
                            {{ $t('publicPages.folder.open') }}
                        </a>
                    </UiButton>
                    <UiButton as-child>
                        <a :href="withPassword(shownFile, shownFile.directUrl, true)" download>
                            <IDownload />
                            {{ $t('publicPages.folder.download') }}
                        </a>
                    </UiButton>
                </UiDialogFooter>
            </UiDialogContent>
        </UiDialog>

        <UiDialog :open="!!unlockTarget" @update:open="!$event && (unlockTarget = null)">
            <UiDialogContent v-if="shownUnlock" class="sm:max-w-sm">
                <form class="flex flex-col gap-6" @submit.prevent="handleUnlock">
                    <UiDialogHeader>
                        <UiDialogTitle class="flex items-center gap-2 pr-10">
                            <ILock class="size-4 shrink-0" />
                            <span class="truncate">{{ shownUnlock.name }}</span>
                        </UiDialogTitle>
                        <UiDialogDescription>
                            {{ $t('publicPages.folder.unlockDescription') }}
                        </UiDialogDescription>
                    </UiDialogHeader>
                    <UiField :data-invalid="!!unlockError">
                        <UiFieldLabel for="unlock-password">
                            {{ $t('publicPages.folder.password') }}
                        </UiFieldLabel>
                        <PasswordInput
                            id="unlock-password"
                            v-model="unlockPassword"
                            autofocus
                            autocomplete="off"
                        />
                        <UiFieldError v-if="unlockError" :errors="[unlockError]" />
                    </UiField>
                    <UiDialogFooter>
                        <UiButton type="button" variant="outline" @click="unlockTarget = null">
                            {{ $t('common.cancel') }}
                        </UiButton>
                        <UiButton type="submit" :disabled="!unlockPassword || isUnlocking">
                            <UiSpinner v-if="isUnlocking" />
                            {{ $t('publicPages.folder.open') }}
                        </UiButton>
                    </UiDialogFooter>
                </form>
            </UiDialogContent>
        </UiDialog>
    </div>
</template>

<script setup lang="ts">
import { ICircleAlert, IFolderOpen, ISearchX } from '#components';

import type { FileData } from '~/types/files';
import type { FolderData } from '~/types/folders';

const route = useRoute();
const { t } = useI18n();

type PublicFile = FileData & { isLocked: boolean };

const { data, error } = await useFetch<FolderData & { embed: IEmbed; lockedCount: number }>(
    `/api/folders/${(route.params as any).id}`,
    {
        headers: useRequestHeaders(['host']),
    },
);

if (error.value) {
    throw createError({ status: 404, message: t('publicPages.folder.notFound') });
}

const search = ref('');
const aiSearch = ref(false);
const typeFilter = ref<FileCategory[]>([]);
const sort = ref('');
const isDownloading = ref(false);
const current = ref<FileData | null>(null);
const unlocked = ref<Record<string, { file: FileData; password: string }>>({});
const unlockTarget = ref<PublicFile | null>(null);
const shownFile = ref(current.value);
const shownUnlock = ref(unlockTarget.value);
const unlockPassword = ref('');
const unlockError = ref('');
const isUnlocking = ref(false);

const subtitle = computed(() => {
    const files = t('publicPages.folder.fileCount', data.value!.fileCount);

    return data.value!.lockedCount
        ? t('publicPages.folder.summaryLocked', { files, locked: data.value!.lockedCount })
        : files;
});

const sortOptions = computed(() => [
    ...(search.value.trim()
        ? [{ label: t('publicPages.folder.sort.relevance'), value: 'relevance' }]
        : []),
    { label: t('publicPages.folder.sort.newest'), value: 'createdAt:desc' },
    { label: t('publicPages.folder.sort.oldest'), value: 'createdAt:asc' },
    { label: t('publicPages.folder.sort.nameAsc'), value: 'name:asc' },
    { label: t('publicPages.folder.sort.nameDesc'), value: 'name:desc' },
    { label: t('publicPages.folder.sort.largest'), value: 'size:desc' },
    { label: t('publicPages.folder.sort.smallest'), value: 'size:asc' },
]);

const sortValue = computed({
    get: () =>
        (sort.value === 'relevance' && !search.value.trim() ? '' : sort.value) ||
        (search.value.trim() ? 'relevance' : 'createdAt:desc'),
    set: (value) => (sort.value = value),
});

const {
    items: files,
    isLoading,
    isError,
    isFetching,
    isLoadingMore,
    loadMore,
    refresh,
} = usePaginatedList<PublicFile>('files', `/api/folders/${data.value!.id}/files`, () => ({
    q: search.value.trim(),
    mode: search.value.trim() && aiSearch.value ? 'vector' : undefined,
    sort: sortValue.value === 'relevance' ? undefined : sortValue.value,
    type: typeFilter.value,
}));

const isFiltered = computed(() => !!search.value.trim() || !!typeFilter.value.length);

const toggleType = (type: FileCategory) => {
    typeFilter.value = typeFilter.value.includes(type)
        ? typeFilter.value.filter((t) => t !== type)
        : [...typeFilter.value, type];
};

const handleClearFilters = () => {
    search.value = '';
    typeFilter.value = [];
};

const visibleFiles = computed(() =>
    files.value.map((file) =>
        unlocked.value[file.id] ? { ...unlocked.value[file.id]!.file, isLocked: false } : file,
    ),
);

const isMedia = (file: FileData) =>
    ['image/', 'video/', 'audio/'].some((type) => file.mimeType.startsWith(type));

const media = computed(() => visibleFiles.value.filter((f) => !f.isLocked && isMedia(f)));

const withPassword = (file: FileData, url: string, download = false) => {
    const params = new URLSearchParams();
    const password = unlocked.value[file.id]?.password;

    if (password) params.set('password', password);
    if (download) params.set('download', '');

    return params.size ? `${url}?${params.toString().replace(/=$/, '')}` : url;
};

watch(current, (file) => file && (shownFile.value = file));
watch(unlockTarget, (file) => file && (shownUnlock.value = file));

const index = computed(() => media.value.findIndex((f) => f.id === current.value?.id));
const prev = computed(() => media.value[index.value - 1]);
const next = computed(() => media.value[index.value + 1]);

const handleOpen = (file: PublicFile) => {
    if (file.isLocked) {
        unlockTarget.value = file;
        unlockPassword.value = '';
        unlockError.value = '';
        return;
    }

    if (isMedia(file)) current.value = file;
    else window.open(withPassword(file, file.embedUrl), '_blank');
};

const handleUnlock = async () => {
    const target = unlockTarget.value!;

    isUnlocking.value = true;
    unlockError.value = '';

    try {
        const file = await $fetch<FileData>(`/api/files/${target.id}`, {
            query: { password: unlockPassword.value },
        });

        unlocked.value = {
            ...unlocked.value,
            [target.id]: { file, password: unlockPassword.value },
        };
        unlockTarget.value = null;
        handleOpen({ ...file, isLocked: false });
    } catch (error: any) {
        unlockError.value = getErrorMessage(error);
    }

    isUnlocking.value = false;
};

const handleDownload = async () => {
    isDownloading.value = true;
    await downloadFolder(data.value as never);
    isDownloading.value = false;
};

onKeyStroke(['ArrowLeft', 'ArrowRight'], (event) => {
    if (!current.value) return;

    const target = event.key === 'ArrowLeft' ? prev.value : next.value;
    if (target) current.value = target;
});
</script>
