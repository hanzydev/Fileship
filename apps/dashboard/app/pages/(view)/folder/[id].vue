<template>
    <div class="flex min-h-svh flex-col">
        <Head>
            <Title>{{ data!.name }}</Title>
        </Head>

        <ViewHeader
            :title="data!.name"
            :subtitle="pluralize(files.length, 'file')"
            content-class="max-w-7xl"
        >
            <UiButton size="sm" :disabled="!files.length || isDownloading" @click="handleDownload">
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
                    :loading="isSearching"
                    ai-available
                />
                <UiSelect v-model="typeFilter">
                    <UiSelectTrigger class="w-40">
                        <IListFilter />
                        <UiSelectValue />
                    </UiSelectTrigger>
                    <UiSelectContent position="popper">
                        <UiSelectItem value="all">All types</UiSelectItem>
                        <UiSelectSeparator />
                        <UiSelectItem
                            v-for="(category, key) in FILE_CATEGORIES"
                            :key="key"
                            :value="key"
                        >
                            {{ category.label }}
                        </UiSelectItem>
                    </UiSelectContent>
                </UiSelect>
            </div>

            <EmptyState
                v-if="!filtered.length"
                :icon="files.length ? ISearchX : IFolderOpen"
                :title="files.length ? 'No matching files' : 'This folder is empty'"
            />

            <VirtualList
                v-else
                :items="filtered"
                :item-key="(f) => f.id"
                :min-item-width="200"
                :estimate-size="240"
                :gap="12"
            >
                <template #default="{ item }">
                    <button
                        type="button"
                        class="group flex flex-col gap-2 rounded-3xl p-2 text-left transition-colors hover:bg-muted"
                        @click="handleOpen(item)"
                    >
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
                        <div class="flex flex-col px-1">
                            <TextBlock variant="small" class="truncate">
                                {{ item.fileName }}
                            </TextBlock>
                            <TextBlock variant="muted" class="text-xs">
                                {{ item.size.formatted }}
                            </TextBlock>
                        </div>
                    </button>
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
                            variant="secondary"
                            size="icon"
                            :disabled="!prev"
                            @click="current = prev!"
                        >
                            <IChevronLeft />
                        </UiButton>
                    </div>
                    <div class="absolute top-1/2 right-0 -translate-y-1/2">
                        <UiButton
                            variant="secondary"
                            size="icon"
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

const { data, error } = await useFetch<
    Omit<FolderData, 'files'> & { files: FileData[]; embed: IEmbed }
>(`/api/folders/${(route.params as any).id}`, {
    headers: useRequestHeaders(['host']),
});

if (error.value) {
    throw createError({ statusCode: 404, message: 'Folder not found' });
}

const search = ref('');
const aiSearch = ref(false);
const typeFilter = ref('all');
const isSearching = ref(false);
const isDownloading = ref(false);
const searchResults = ref<string[] | null>(null);
const current = ref<FileData | null>(null);

const files = computed(() => data.value!.files);

const filtered = computed(() =>
    files.value.filter(
        (f) =>
            (typeFilter.value === 'all' || getFileCategory(f) === typeFilter.value) &&
            (!searchResults.value || searchResults.value.includes(f.id)),
    ),
);

const media = computed(() =>
    filtered.value.filter((f) =>
        ['image/', 'video/', 'audio/'].some((t) => f.mimeType.startsWith(t)),
    ),
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

const handleSearch = async () => {
    if (!search.value.trim()) {
        searchResults.value = null;
        return;
    }

    isSearching.value = true;

    try {
        searchResults.value = await $fetch<string[]>(`/api/folders/${data.value!.id}/searchFiles`, {
            method: 'POST',
            body: { query: search.value.trim(), mode: aiSearch.value ? 'vector' : 'fulltext' },
        });
    } catch {
        searchResults.value = [];
    }

    isSearching.value = false;
};

watchDebounced(search, handleSearch, { debounce: 400 });
watch(aiSearch, handleSearch);

onKeyStroke(['ArrowLeft', 'ArrowRight'], (event) => {
    if (!current.value) return;

    const target = event.key === 'ArrowLeft' ? prev.value : next.value;
    if (target) current.value = target;
});
</script>
