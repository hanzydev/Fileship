<template>
    <Head>
        <Title>Files</Title>
    </Head>

    <ModalsViewFile
        v-if="viewFileModal.fileId"
        v-model="viewFileModal.open"
        :file-id="viewFileModal.fileId"
    />

    <ModalsAreYouSure
        v-model="areYouSureModalOpen"
        :title="`Delete Selected Files (${selectedFiles.length})`"
        description="Are you sure you want to delete the selected files?"
        @confirm="handleBulkDelete"
    />

    <DashboardContent>
        <template #header>
            <h2 lt-md="text-2xl!">Files</h2>
            <div flex="~ items-center gap-2" mla lt-md="hidden">
                <Transition
                    enter-active-class="motion-safe:(animate-in fade-in zoom-in-95)"
                    leave-active-class="motion-safe:(animate-out fade-out zoom-out-95)"
                >
                    <UiButton
                        v-if="selectionMode && selectedFiles.length && files.length"
                        icon="solar:trash-bin-minimalistic-bold"
                        icon-size="20"
                        alignment="center"
                        variant="dangerFill"
                        gap-2
                        rounded-xl="!"
                        :disabled="bulkDeleting"
                        :loading="bulkDeleting"
                        @click="areYouSureModalOpen = true"
                    >
                        Delete Selected ({{ selectedFiles.length }})
                    </UiButton>
                </Transition>
                <Transition
                    enter-active-class="motion-safe:(animate-in fade-in zoom-in-95)"
                    leave-active-class="motion-safe:(animate-out fade-out zoom-out-95)"
                >
                    <UiButton
                        v-if="selectionMode && files.length"
                        :icon="
                            selectedFiles.length === files.length
                                ? 'solar:list-cross-minimalistic-bold'
                                : 'solar:list-check-minimalistic-bold'
                        "
                        icon-size="20"
                        alignment="center"
                        variant="primary"
                        gap-2
                        rounded-xl="!"
                        :disabled="bulkDeleting"
                        @click="
                            selectedFiles.length === files.length
                                ? (selectedFiles = [])
                                : (selectedFiles = files.map((f) => f.id))
                        "
                    >
                        {{ selectedFiles.length === files.length ? 'Deselect All' : 'Select All' }}
                    </UiButton>
                </Transition>
                <Transition
                    enter-active-class="motion-safe:(animate-in fade-in zoom-in-95)"
                    leave-active-class="motion-safe:(animate-out fade-out zoom-out-95)"
                >
                    <UiButton
                        v-if="files.length"
                        :icon="
                            selectionMode ? 'solar:close-square-bold' : 'solar:check-square-bold'
                        "
                        icon-size="20"
                        alignment="center"
                        variant="accent"
                        gap-2
                        rounded-xl="!"
                        :disabled="bulkDeleting"
                        @click="selectionMode = !selectionMode"
                    >
                        {{ selectionMode ? 'Exit Selection Mode' : 'Enter Selection Mode' }}
                    </UiButton>
                </Transition>
            </div>
        </template>
        <div flex="~ gap4 1 items-center <sm:col" wfull>
            <UiSearchBar
                v-model="searchQuery"
                v-model:ai-enabled="aiEnabled"
                v-model:loading="isSearching"
                placeholder="Search files..."
                :ai-available="currentUser?.aiSettings?.enabled"
                wfull
            />
            <FileTypeFilter v-model="filterType" />
        </div>
        <div grid="~ gap6 lg:cols-3 md:cols-2 xl:cols-4">
            <New h208px @action="router.push('/dashboard/files/upload')" />

            <template v-if="isLoading">
                <UiSkeletonCard
                    v-for="i in randomNumber(3, 7)"
                    :key="i"
                    flex="~ col items-center justify-center gap2"
                    h208px
                >
                    <Icon
                        v-if="randomNumber(0, 1) === 0"
                        name="solar:play-bold"
                        size="64"
                        animate-pulse
                        op75
                    />
                    <Icon v-else name="solar:gallery-bold" size="64" animate-pulse op75 />
                </UiSkeletonCard>
            </template>
            <TransitionGroup
                v-else
                :css="false"
                @enter="(el, done) => (isAnimating ? done() : enter(el, done))"
                @leave="(el, done) => (isAnimating ? done() : leave(el, done))"
            >
                <div v-for="file in calculatedFiles" :key="file.id" op0 class="fileCard">
                    <FileCard
                        :data="file"
                        :selectable="selectionMode && files.length > 0"
                        :selected="selectionMode && selectedFiles.includes(file.id)"
                        @view-file="
                            (file) => {
                                viewFileModal.fileId = file.id;
                                nextTick(() => (viewFileModal.open = true));
                            }
                        "
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
        <UiPagination v-model="currentPage" :item-count="filtered.length" :items-per-page="19" />
    </DashboardContent>
</template>

<script setup lang="ts">
const files = useFiles();
const folders = useFolders();
const router = useRouter();
const currentUser = useAuthUser();
const { $toast } = useNuxtApp();

const currentPage = ref(1);
const filterType = ref([]);
const searchQuery = ref('');
const aiEnabled = ref(false);
const searched = ref<string[] | null>(null);

const selectionMode = ref(false);
const bulkDeleting = ref(false);
const areYouSureModalOpen = ref(false);
const selectedFiles = ref<string[]>([]);

const viewFileModal = reactive({ open: false, fileId: null as string | null });

const filtered = computed(() =>
    files.value.filter((f) => {
        if (f.folderId) return false;

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

        return isTypeMatch && isSearchMatch;
    }),
);

const calculatedFiles = computed(() => {
    const start = (currentPage.value - 1) * 19;
    const end = start + 19;
    return filtered.value.slice(start, end);
});

const isAnimating = ref(false);
const isSearching = ref(false);
const isLoading = ref(files.value.length !== currentUser.value!.stats.files);

const { all, enter, leave } = animateCards();

const handleBulkDelete = async () => {
    bulkDeleting.value = true;

    try {
        await $fetch('/api/files/bulk-delete', {
            method: 'POST',
            body: { files: selectedFiles.value },
        });

        $toast.success(`${selectedFiles.value.length} files deleted successfully`);

        selectedFiles.value = [];
        selectionMode.value = false;
    } catch (error: any) {
        $toast.error(error.data.message);
    }

    bulkDeleting.value = false;
};

onMounted(async () => {
    if (isLoading.value) {
        const foldersData = await $fetch('/api/folders');
        const filesData = await $fetch('/api/files');

        folders.value = foldersData.map((f) => ({
            ...f,
            createdAt: new Date(f.createdAt),
        }));

        files.value = filesData.map((f) => ({
            ...f,
            expiresAt: f.expiresAt ? new Date(f.expiresAt) : null,
            createdAt: new Date(f.createdAt),
        }));

        isLoading.value = false;
        await nextTick();
    }

    all('files', '.fileCard');
});

let searchTimeout: NodeJS.Timeout;

watch(currentPage, () => {
    isAnimating.value = true;
    nextTick(() => {
        all('files', '.fileCard', () => {
            isAnimating.value = false;
        });
    });
});

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
        all('files', '.fileCard', () => {
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

definePageMeta({
    layout: 'dashboard',
    middleware: 'file-uploader-only',
});
</script>
