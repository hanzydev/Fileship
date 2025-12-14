<template>
    <Head>
        <Title>Folders</Title>
    </Head>

    <ModalsCreateFolder v-model="createFolderModalOpen" />

    <DashboardContent>
        <template #header>
            <h2 lt-md="text-2xl!">Folders</h2>
        </template>

        <UiSearchBar
            v-model="searchQuery"
            v-model:loading="isSearching"
            placeholder="Search folders..."
        />
        <div grid="~ gap6 lg:cols-3 md:cols-2 xl:cols-4 2xl:cols-5">
            <New h164px @action="createFolderModalOpen = true" />

            <template v-if="isLoading">
                <UiSkeletonCard
                    v-for="i in randomNumber(3, 7)"
                    :key="i"
                    h164px
                    flex="~ col gap4 justify-between"
                >
                    <UiSkeletonLine
                        h5
                        :style="{
                            width: `${randomNumber(30, 70)}%`,
                        }"
                    />
                    <div text-fs-muted-2 space-y-2>
                        <UiSkeletonLine h4 w16 />
                        <UiSkeletonLine h4 w14 />
                        <UiSkeletonLine h4 w40 />
                    </div>
                </UiSkeletonCard>
            </template>
            <TransitionGroup
                v-else
                :css="false"
                @enter="(el, done) => (isAnimating ? done() : enter(el, done))"
                @leave="(el, done) => (isAnimating ? done() : leave(el, done))"
            >
                <div v-for="folder in calculatedFolders" :key="folder.id" op0 class="folderCard">
                    <FolderCard :data="folder" />
                </div>
            </TransitionGroup>
        </div>
        <UiPagination v-model="currentPage" :item-count="filtered.length" :items-per-page="19" />
    </DashboardContent>
</template>

<script setup lang="ts">
const folders = useFolders();
const files = useFiles();
const currentUser = useAuthUser();

const currentPage = ref(1);
const searchQuery = ref('');
const searched = ref<string[]>([]);

const createFolderModalOpen = ref(false);

const filtered = computed(() =>
    folders.value.filter((f) =>
        !isSearching.value && searchQuery.value.length ? searched.value.includes(f.id) : true,
    ),
);

const calculatedFolders = computed(() => {
    const start = (currentPage.value - 1) * 19;
    const end = start + 19;
    return filtered.value.slice(start, end);
});

const isAnimating = ref(false);
const isSearching = ref(false);
const isLoading = ref(folders.value.length !== currentUser.value!.stats.folders);

const { all, enter, leave } = animateCards();

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

    all('folders', '.folderCard');
});

let searchTimeout: NodeJS.Timeout;

watch(currentPage, () => {
    isAnimating.value = true;
    nextTick(() => {
        all('folders', '.folderCard', () => {
            isAnimating.value = false;
        });
    });
});

watch(searchQuery, (query) => {
    clearTimeout(searchTimeout);

    if (query.length) {
        isSearching.value = true;

        searchTimeout = setTimeout(async () => {
            isAnimating.value = true;

            try {
                searched.value = await $fetch<string[]>('/api/folders/search', {
                    method: 'POST',
                    body: { query },
                });
            } catch {
                searched.value = [];
            }

            isSearching.value = false;

            nextTick(() => {
                all('folders', '.folderCard', () => {
                    isAnimating.value = false;
                });
            });
        }, 750);
    } else {
        searched.value = [];
    }
});

definePageMeta({
    layout: 'dashboard',
    middleware: 'file-uploader-only',
});
</script>
