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
            <UiSearchBar
                v-model="searchQuery"
                placeholder="Search files..."
                wfull
            />
            <FileTypeFilter v-model="filterType" />
        </div>

        <div v-if="filtered.length" grid="~ gap6 lg:cols-3 md:cols-2 xl:cols-4">
            <template v-for="file in calculatedFiles" :key="file.id">
                <FileCard
                    :selected="editable && selectedFiles.includes(file.id)"
                    :data="file"
                    :selectable="editable"
                    @update:selected="
                        (value) => {
                            if (value) {
                                selectedFiles.push(file.id);
                            } else {
                                selectedFiles.splice(
                                    selectedFiles.indexOf(file.id),
                                    1,
                                );
                            }
                        }
                    "
                />
            </template>
        </div>
        <NothingHere
            v-else
            message="There are no files to display."
            icon="heroicons-solid:document-duplicate"
        />
        <UiPagination
            v-model="currentPage"
            :item-count="filtered.length"
            :items-per-page="20"
        />

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
            @click="saveChanges"
        >
            Save
        </UiButton>
    </UiModal>
</template>

<script setup lang="ts">
import { toast } from 'vue-sonner';

const isOpen = defineModel<boolean>({ required: true });

const { data, editable } = defineProps<{
    data: FolderData;
    editable?: boolean;
}>();

const files = useFiles();

const searchQuery = ref('');
const currentPage = ref(1);
const filterType = ref([]);
const disabled = ref(false);

const selectedFiles = ref(data.files.map((f) => f.id));

const filtered = computed(() =>
    files.value
        .filter((f) =>
            editable
                ? [null, data.id].includes(f.folderId)
                : f.folderId === data.id,
        )
        .filter(
            (f) =>
                !filterType.value.length ||
                filterType.value.some((t) => f.mimeType.startsWith(`${t}/`)),
        )
        .filter((f) =>
            Object.values(f).some((v) =>
                v
                    ?.toString()
                    ?.toLowerCase()
                    ?.includes(searchQuery.value.toLowerCase()),
            ),
        ),
);

const calculatedFiles = computed(() => {
    const start = (currentPage.value - 1) * 20;
    const end = start + 20;
    return filtered.value.slice(start, end);
});

const saveChanges = async () => {
    disabled.value = true;

    await $fetch(`/api/folders/${data.id}`, {
        method: 'PATCH',
        body: {
            files: files.value
                .filter((f) => selectedFiles.value.includes(f.id))
                .map((f) => f.id),
        },
    });

    disabled.value = false;
    isOpen.value = false;

    toast.success('Files saved successfully');
};

watch(
    () => data.files,
    (value) => {
        selectedFiles.value = value.map((f) => f.id);
    },
);
</script>
