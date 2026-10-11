<template>
    <div class="flex min-h-svh flex-col">
        <Head>
            <Title>{{ data!.name }}</Title>
        </Head>

        <ViewHeader
            :title="data!.name"
            :subtitle="pluralize(data!.fileCount, 'file')"
            content-class="max-w-7xl"
        >
            <UiButton
                size="sm"
                :disabled="!data!.fileCount || isDownloading"
                @click="handleDownload"
            >
                <UiSpinner v-if="isDownloading" />
                <IDownload v-else />
                <span class="hidden sm:inline">Download all</span>
            </UiButton>
        </ViewHeader>

        <main class="mx-auto flex w-full max-w-7xl flex-1 flex-col gap-4 p-4">
            <div class="flex flex-wrap gap-2">
                <SmartSearchInput
                    v-model="search"
                    v-model:ai="aiSearch"
                    class="min-w-56 flex-1"
                    placeholder="Search files..."
                    :loading="!!search && isLoading"
                    ai-available
                />
                <UiDropdownMenu>
                    <UiDropdownMenuTrigger as-child>
                        <UiButton variant="outline">
                            <IListFilter />
                            {{
                                typeFilter === 'all'
                                    ? 'All types'
                                    : FILE_CATEGORIES[typeFilter as FileCategory].label
                            }}
                            <IChevronDown class="opacity-50" />
                        </UiButton>
                    </UiDropdownMenuTrigger>
                    <UiDropdownMenuContent align="end" class="w-48">
                        <UiDropdownMenuLabel>File type</UiDropdownMenuLabel>
                        <UiDropdownMenuRadioGroup v-model="typeFilter">
                            <UiDropdownMenuRadioItem value="all">All types</UiDropdownMenuRadioItem>
                            <UiDropdownMenuSeparator />
                            <UiDropdownMenuRadioItem
                                v-for="(category, key) in FILE_CATEGORIES"
                                :key="key"
                                :value="key"
                            >
                                <component :is="category.icon" />
                                {{ category.label }}
                            </UiDropdownMenuRadioItem>
                        </UiDropdownMenuRadioGroup>
                    </UiDropdownMenuContent>
                </UiDropdownMenu>
            </div>

            <div
                v-if="isLoading"
                class="grid grid-cols-[repeat(auto-fill,minmax(200px,1fr))] gap-3"
            >
                <FileCardSkeleton v-for="i in 8" :key="i" />
            </div>

            <EmptyState
                v-else-if="!files.length"
                :icon="data!.fileCount ? ISearchX : IFolderOpen"
                :title="data!.fileCount ? 'No matching files' : 'This folder is empty'"
            />

            <VirtualList
                v-else
                :items="files"
                :item-key="(f) => f.id"
                :min-item-width="200"
                :estimate-size="240"
                :gap="12"
                :loading="isLoadingMore"
                @reach-end="loadMore"
            >
                <template #default="{ item }">
                    <UiCard size="sm" class="group" @click="handleOpen(item)">
                        <UiCardContent>
                            <div class="relative aspect-4/3 overflow-hidden rounded-2xl bg-muted">
                                <img
                                    v-if="item.mimeType.startsWith('image/') || item.thumbnailUrl"
                                    :src="item.thumbnailUrl || item.directUrl"
                                    :alt="item.fileName"
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
                                    v-if="item.mimeType.startsWith('video/')"
                                    class="absolute inset-0 flex items-center justify-center"
                                >
                                    <IPlay class="size-8 fill-white text-white drop-shadow" />
                                </div>
                            </div>
                        </UiCardContent>
                        <UiCardFooter class="flex-col items-start gap-0.5">
                            <TextBlock variant="small" class="w-full truncate">
                                {{ item.fileName }}
                            </TextBlock>
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
                v-if="current"
                class="flex h-[90svh] max-w-[95vw] flex-col gap-4 sm:max-w-[95vw]"
            >
                <UiDialogHeader>
                    <UiDialogTitle class="truncate pr-10">{{ current.fileName }}</UiDialogTitle>
                    <UiDialogDescription>{{ current.size.formatted }}</UiDialogDescription>
                </UiDialogHeader>
                <div class="relative flex min-h-0 flex-1 items-center justify-center">
                    <img
                        v-if="current.mimeType.startsWith('image/')"
                        :src="current.directUrl"
                        :alt="current.fileName"
                        class="max-h-full max-w-full rounded-2xl object-contain"
                    />
                    <video
                        v-else-if="current.mimeType.startsWith('video/')"
                        :key="current.id"
                        :src="current.directUrl"
                        :poster="current.thumbnailUrl || undefined"
                        controls
                        autoplay
                        class="max-h-full max-w-full rounded-2xl"
                    />
                    <audio v-else :key="current.id" :src="current.directUrl" controls autoplay />

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
                        <a :href="current.embedUrl" target="_blank">
                            <IExternalLink />
                            Open
                        </a>
                    </UiButton>
                    <UiButton as-child>
                        <a :href="`${current.directUrl}?download`" download>
                            <IDownload />
                            Download
                        </a>
                    </UiButton>
                </UiDialogFooter>
            </UiDialogContent>
        </UiDialog>
    </div>
</template>

<script setup lang="ts">
import { IFolderOpen, ISearchX } from '#components';

const route = useRoute();

const { data, error } = await useFetch<FolderData & { embed: IEmbed }>(
    `/api/folders/${(route.params as any).id}`,
    {
        headers: useRequestHeaders(['host']),
    },
);

if (error.value) {
    throw createError({ statusCode: 404, message: 'Folder not found' });
}

const search = ref('');
const aiSearch = ref(false);
const typeFilter = ref('all');
const isDownloading = ref(false);
const current = ref<FileData | null>(null);

const {
    items: files,
    isLoading,
    isLoadingMore,
    loadMore,
} = usePaginatedList<FileData>('files', `/api/folders/${data.value!.id}/files`, () => ({
    q: search.value.trim(),
    mode: search.value.trim() && aiSearch.value ? 'vector' : undefined,
    type: typeFilter.value === 'all' ? undefined : typeFilter.value,
}));

const media = computed(() =>
    files.value.filter((f) => ['image/', 'video/', 'audio/'].some((t) => f.mimeType.startsWith(t))),
);

const index = computed(() => media.value.findIndex((f) => f.id === current.value?.id));
const prev = computed(() => media.value[index.value - 1]);
const next = computed(() => media.value[index.value + 1]);

const handleOpen = (file: FileData) => {
    if (['image/', 'video/', 'audio/'].some((t) => file.mimeType.startsWith(t)))
        current.value = file;
    else window.open(file.embedUrl, '_blank');
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
