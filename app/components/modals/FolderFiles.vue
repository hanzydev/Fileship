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

        <div flex="~ gap4 1 items-center" wfull>
            <UiSearchBar v-model="searchQuery" placeholder="Search files..." wfull />
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
import { useFuse } from '@vueuse/integrations/useFuse';

const isOpen = defineModel<boolean>({ required: true });

const { data, editable } = defineProps<{
    data: FolderData;
    editable?: boolean;
}>();

const files = useFiles();
const { $toast } = useNuxtApp();

const searchQuery = ref('');
const currentPage = ref(1);
const filterType = ref([]);
const disabled = ref(false);

const selectedFiles = ref(data.files);

const { results } = useFuse(
    searchQuery,
    computed(() =>
        editable
            ? files.value.filter((f) => [null, data.id].includes(f.folderId))
            : files.value.filter((f) => selectedFiles.value.includes(f.id)),
    ),
    {
        matchAllWhenSearchEmpty: true,
        fuseOptions: {
            keys: [
                {
                    name: 'fileName',
                    weight: 2,
                },
                'mimeType',
            ],
        },
    },
);

const filtered = computed<FileData[]>(() =>
    results.value
        .map((r) => r.item)
        .filter(
            (f) =>
                !filterType.value.length ||
                filterType.value.some((t) => f.mimeType.startsWith(`${t}/`)),
        ),
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

const isAnimating = ref(false);
const { all, enter, leave } = animateCards();

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
</script>
