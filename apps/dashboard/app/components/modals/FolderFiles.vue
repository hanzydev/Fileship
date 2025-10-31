<template>
    <UiModal
        v-model="isOpen"
        max-hscreen
        max-wfull
        min-hscreen
        wscreen="!"
        rounded-none
        p8
        space-y-6
        ring-0="!"
        background-class="hidden!"
        :close-on-outer-click="false"
    >
        <div flex="~ items-center justify-between">
            <h2>
                {{ editable ? 'Select Files' : 'Files' }}
            </h2>
            <UiButton
                icon="heroicons-solid:x"
                absolute
                right-8
                top-8
                h10
                w10
                p0="!"
                icon-size="20"
                alignment="center"
                :disabled
                @click="isOpen = false"
            />
        </div>

        <div flex="~ gap4 1 items-center <sm:col" wfull>
            <UiSearchBar
                v-model="searchQuery"
                v-model:ai-enabled="aiEnabled"
                v-model:loading="isSearching"
                placeholder="Search files..."
                ai-available
                wfull
            />
            <FileTypeFilter v-model="filterType" />
        </div>

        <div v-show="filtered.length" grid="~ gap6 lg:cols-3 md:cols-2 xl:cols-4">
            <TransitionGroup
                :css="false"
                @enter="(el, done) => (isAnimating ? done() : enter(el, done))"
                @leave="(el, done) => (isAnimating ? done() : leave(el, done))"
            >
                <div v-for="file in calculatedFiles" :key="file.id" op0 class="folderFileCard">
                    <FileCard
                        :selected="editable && selectedFiles.includes(file.id)"
                        :data="file"
                        :selectable="editable"
                        @update:selected="
                            (value) => {
                                if (value) {
                                    selectedFiles.push(file.id);
                                } else {
                                    selectedFiles.splice(selectedFiles.indexOf(file.id), 1);
                                }
                            }
                        "
                    />
                </div>
            </TransitionGroup>
        </div>
        <NothingHere
            v-if="!filtered.length"
            message="There are no files to display."
            icon="heroicons-solid:document-duplicate"
        />
        <UiPagination v-model="currentPage" :item-count="filtered.length" :items-per-page="20" />

        <UiButton
            v-if="editable"
            wfull
            gap2
            alignment="center"
            variant="accent"
            icon="heroicons:pencil-16-solid"
            icon-size="20"
            :loading="disabled"
            :disabled
            @click="handleChange"
        >
            Save
        </UiButton>
    </UiModal>
</template>

<script setup lang="ts">
const isOpen = defineModel<boolean>({ required: true });

const { data, editable } = defineProps<{
    data: FolderData;
    editable?: boolean;
}>();

const files = useFiles();
const { $toast } = useNuxtApp();

const currentPage = ref(1);
const filterType = ref([]);
const searchQuery = ref('');
const aiEnabled = ref(false);
const searched = ref<string[] | null>(null);
const disabled = ref(false);

const selectedFiles = ref(data.files);

const filtered = computed(() =>
    files.value.filter((f) => {
        const isSelected = selectedFiles.value.includes(f.id);
        const isInFolderOrNot = [null, data.id].includes(f.folderId);
        const isSearchMatch = searched.value === null ? true : searched.value.includes(f.id);

        const checks = {
            image: f.mimeType.startsWith('image/'),
            video: f.mimeType.startsWith('video/'),
            audio: f.mimeType.startsWith('audio/'),
            document: DOCUMENT_FILE_MIME_TYPES.includes(f.mimeType),
            archive: ARCHIVE_FILE_MIME_TYPES.includes(f.mimeType),
            code: CODE_FILE_EXTENSIONS.includes(getExtname(f.fileName)),
        };

        let isTypeMatch = true;
        if (filterType.value.length) {
            isTypeMatch = false;
            for (const type of filterType.value) {
                if (checks[type as never]) {
                    isTypeMatch = true;
                    break;
                }
            }
        }

        const isFolderOrSelectionMatch = editable ? isInFolderOrNot : isSelected;
        return isFolderOrSelectionMatch && isTypeMatch && isSearchMatch;
    }),
);

const calculatedFiles = computed(() => {
    const start = (currentPage.value - 1) * 20;
    const end = start + 20;
    return filtered.value.slice(start, end);
});

const handleChange = async () => {
    disabled.value = true;

    await $fetch(`/api/folders/${data.id}`, {
        method: 'PATCH',
        body: {
            files: files.value.filter((f) => selectedFiles.value.includes(f.id)).map((f) => f.id),
        },
    });

    disabled.value = false;
    isOpen.value = false;

    $toast.success('Files saved successfully');
};

const isSearching = ref(false);
const isAnimating = ref(false);

const { all, enter, leave } = animateCards();

let searchTimeout: NodeJS.Timeout;

watch(isOpen, () => {
    if (isOpen.value) nextTick(() => all('folderFiles', '.folderFileCard'));
});

watch(currentPage, () => {
    isAnimating.value = true;
    nextTick(() => {
        all('folderFiles', '.folderFileCard', () => {
            isAnimating.value = false;
        });
    });
});

watch(
    () => data.files,
    (value) => (selectedFiles.value = value),
);

const handleSearch = async (query: string) => {
    isAnimating.value = true;

    try {
        searched.value = await $fetch<string[]>('/api/files/search', {
            method: 'POST',
            body: { query, mode: aiEnabled.value ? 'vector' : 'fulltext' },
        });
    } catch {
        searched.value = [];
    }

    isSearching.value = false;

    nextTick(() => {
        all('folderFiles', '.folderFileCard', () => {
            isAnimating.value = false;
        });
    });
};

watch(searchQuery, (query) => {
    clearTimeout(searchTimeout);

    if (query.length) {
        isSearching.value = true;
        searchTimeout = setTimeout(() => handleSearch(query), 750);
    } else {
        searched.value = null;
    }
});

watch(aiEnabled, () => {
    if (searchQuery.value.length) {
        isSearching.value = true;
        searchTimeout = setTimeout(() => handleSearch(searchQuery.value), 100);
    }
});
</script>
