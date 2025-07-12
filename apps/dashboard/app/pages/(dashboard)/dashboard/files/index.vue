<template>
    <div>
        <Head>
            <Title>Files</Title>
        </Head>

        <div space-y-6>
            <h2>Files</h2>
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
                            name="heroicons:play-solid"
                            size="64"
                            animate-pulse
                            op75
                        />
                        <Icon v-else name="heroicons:photo-16-solid" size="64" animate-pulse op75 />
                    </UiSkeletonCard>
                </template>
                <TransitionGroup
                    v-else
                    :css="false"
                    @enter="(el, done) => (isAnimating ? done() : enter(el, done))"
                    @leave="(el, done) => (isAnimating ? done() : leave(el, done))"
                >
                    <div v-for="file in calculatedFiles" :key="file.id" op0 class="fileCard">
                        <FileCard :data="file" />
                    </div>
                </TransitionGroup>
            </div>
            <UiPagination
                v-model="currentPage"
                :item-count="filtered.length"
                :items-per-page="19"
            />
        </div>
    </div>
</template>

<script setup lang="ts">
const files = useFiles();
const folders = useFolders();
const router = useRouter();
const currentUser = useAuthUser();

const currentPage = ref(1);
const filterType = ref([]);
const searchQuery = ref('');
const aiEnabled = ref(false);
const searched = ref<string[]>([]);

const filtered = computed(() =>
    files.value
        .filter((f) =>
            !isSearching.value && searchQuery.value.length ? searched.value.includes(f.id) : true,
        )
        .filter((f) => !f.folderId)
        .filter(
            (f) =>
                !filterType.value.length ||
                filterType.value.some((t) => f.mimeType.startsWith(`${t}/`)),
        ),
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

watch(searchQuery, (query) => {
    clearTimeout(searchTimeout);

    if (query.length) {
        isSearching.value = true;

        searchTimeout = setTimeout(async () => {
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
