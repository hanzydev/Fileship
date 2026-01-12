<template>
    <div>
        <ModalsViewFile
            v-if="viewFileModal.fileId"
            v-model="viewFileModal.open"
            :file-id="viewFileModal.fileId"
        />

        <ModalsEditFolder v-model="editModalOpen" :data />

        <ModalsAreYouSure
            v-model="areYouSureModalOpen"
            title="Delete Folder"
            description="Are you sure you want to delete this folder?"
            @confirm="handleDelete"
        >
            <template #extra>
                <div v-if="data.files.length" flex="~ gap2 items-center">
                    <UiSwitch v-model="deleteFilesToo" :disabled="deleting" />
                    <span text-fs-muted-1 font-medium="!">Delete files too</span>
                </div>
            </template>
        </ModalsAreYouSure>

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
            z30="!"
            background-class="hidden!"
            :close-on-outer-click="false"
        >
            <div flex="~ items-center justify-between">
                <h2>
                    {{ newFolder ? 'Select Files' : editMode ? 'Edit Folder Files' : data.name }}
                </h2>
                <div flex="~ gap2.5" hfit>
                    <div
                        flex="~ items-center justify-center gap-1"
                        ring="1 fs-overlay-4"
                        rounded-2xl
                        bg-fs-overlay-2
                        px1
                    >
                        <UiButton
                            v-if="data.public"
                            alignment="center"
                            variant="onOverlay"
                            class="size-9 shrink-0 text-fs-muted-2 !rounded-xl !p0 hover:text-white"
                            :icon="copied ? 'solar:clipboard-check-bold' : 'solar:clipboard-bold'"
                            :icon-class="copied && 'text-green500!'"
                            icon-size="20"
                            @click="handleCopy"
                        />

                        <UiButton
                            alignment="center"
                            variant="onOverlay"
                            class="size-9 shrink-0 text-fs-muted-2 !rounded-xl !p0 hover:text-white"
                            icon="solar:pen-2-bold"
                            icon-size="24"
                            @click="editModalOpen = true"
                        />
                        <UiButton
                            v-if="!newFolder"
                            alignment="center"
                            variant="onOverlay"
                            class="size-9 shrink-0 text-fs-muted-2 !rounded-xl !p0 hover:text-white"
                            :icon="
                                editMode ? 'solar:gallery-remove-bold' : 'solar:gallery-edit-bold'
                            "
                            icon-size="24"
                            @click="editMode = !editMode"
                        />
                        <UiButton
                            alignment="center"
                            class="size-9 shrink-0 text-fs-muted-2 !rounded-xl !p0 !hover:(bg-red-600 text-white)"
                            variant="onOverlay"
                            icon="solar:trash-bin-minimalistic-bold"
                            icon-size="20"
                            :disabled="deleting"
                            @click="areYouSureModalOpen = true"
                        />
                    </div>

                    <UiButton
                        variant="accent"
                        alignment="center"
                        class="size-11 shrink-0 text-fs-muted-2 !rounded-2xl !p0 hover:text-white"
                        icon="lucide:x"
                        icon-size="24"
                        @click="isOpen = false"
                    />
                </div>
            </div>

            <div flex="~ gap4 1 items-center <sm:col" wfull>
                <UiSearchBar
                    v-model="searchQuery"
                    v-model:ai-enabled="aiEnabled"
                    v-model:loading="isSearching"
                    placeholder="Search files..."
                    :ai-available="currentUser?.aiSettings?.enabled"
                    wfull
                    rounded-2xl="!"
                    :input-class="aiEnabled ? 'rounded-14px!' : 'rounded-2xl!'"
                    ai-toggle-class="rounded-xl!"
                />
                <FileTypeFilter
                    v-model="filterType"
                    dropdown-class="rounded-2xl!"
                    rounded-2xl="!"
                    button-class="rounded-xl!"
                />
            </div>

            <div v-show="filtered.length" grid="~ gap6 lg:cols-3 md:cols-2 xl:cols-4">
                <TransitionGroup
                    :css="false"
                    @enter="(el, done) => (isAnimating ? done() : enter(el, done))"
                    @leave="(el, done) => (isAnimating ? done() : leave(el, done))"
                >
                    <div v-for="file in calculatedFiles" :key="file.id" op0 class="folderFileCard">
                        <FileCard
                            :selected="editMode && selectedFiles.includes(file.id)"
                            :data="file"
                            :selectable="editMode"
                            rounded="2xl"
                            @update:selected="
                                (value) => {
                                    if (value) {
                                        selectedFiles.push(file.id);
                                    } else {
                                        selectedFiles.splice(selectedFiles.indexOf(file.id), 1);
                                    }
                                }
                            "
                            @view-file="
                                (file) => {
                                    viewFileModal.fileId = file.id;
                                    nextTick(() => (viewFileModal.open = true));
                                }
                            "
                        />
                    </div>
                </TransitionGroup>
            </div>
            <NothingHere
                v-if="!filtered.length"
                message="There are no files to display."
                icon="solar:documents-bold"
                rounded-2xl="!"
            />
            <UiPagination
                v-model="currentPage"
                :item-count="filtered.length"
                :items-per-page="20"
                rounded-2xl="!"
                button-class="rounded-xl!"
            />

            <UiButton
                v-if="editMode"
                wfull
                gap2
                alignment="center"
                variant="accent"
                icon="solar:pen-2-bold"
                icon-size="20"
                rounded-2xl="!"
                :loading="disabled"
                :disabled
                @click="handleChange"
            >
                Save
            </UiButton>
        </UiModal>
    </div>
</template>

<script setup lang="ts">
const { data, newFolder } = defineProps<{
    data: FolderData;
    newFolder?: boolean;
}>();

const isOpen = defineModel<boolean>({ required: true });
const editMode = defineModel<boolean>('editMode', { default: false, required: false });

editMode.value = !!newFolder;

const files = useFiles();
const { $toast } = useNuxtApp();
const currentUser = useAuthUser();
const { copied, copy } = useClipboard({ legacy: true });

const currentPage = ref(1);
const filterType = ref([]);
const searchQuery = ref('');
const aiEnabled = ref(false);
const searched = ref<string[] | null>(null);
const disabled = ref(false);

const selectedFiles = ref(data.files);
const viewFileModal = reactive({ open: false, fileId: null as string | null });

const editModalOpen = ref(false);
const deleting = ref(false);
const areYouSureModalOpen = ref(false);
const deleteFilesToo = ref(true);

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

        const isFolderOrSelectionMatch = editMode.value ? isInFolderOrNot : isSelected;
        return isFolderOrSelectionMatch && isTypeMatch && isSearchMatch;
    }),
);

const calculatedFiles = computed(() => {
    const start = (currentPage.value - 1) * 20;
    const end = start + 20;
    return filtered.value.slice(start, end);
});

const handleDelete = async () => {
    deleting.value = true;

    try {
        await $fetch(`/api/folders/${data.id}`, {
            method: 'DELETE',
            body: { deleteFilesToo: deleteFilesToo.value },
        });

        $toast.success(
            deleteFilesToo.value
                ? 'Folder and its files deleted successfully'
                : 'Folder deleted successfully',
        );
    } catch (error: any) {
        $toast.error(error.data.message);
    }

    deleting.value = false;
};

const handleCopy = () => {
    copy(data.publicUrl!);
    $toast.success('Link copied to clipboard');
};

const handleChange = async () => {
    disabled.value = true;

    await $fetch(`/api/folders/${data.id}`, {
        method: 'PATCH',
        body: {
            files: files.value.filter((f) => selectedFiles.value.includes(f.id)).map((f) => f.id),
        },
    });

    disabled.value = false;
    editMode.value = false;

    if (newFolder) isOpen.value = false;

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
