<template>
    <div>
        <Head>
            <Title>Files</Title>
        </Head>

        <div space-y-6>
            <h2>Files</h2>
            <div flex="~ gap4 1 items-center" wfull>
                <UiSearchBar
                    v-model="searchQuery"
                    placeholder="Search files..."
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
                        <Icon
                            v-else
                            name="heroicons:photo-16-solid"
                            size="64"
                            animate-pulse
                            op75
                        />
                    </UiSkeletonCard>
                </template>
                <TransitionGroup
                    v-else
                    :css="false"
                    @enter="
                        (el, done) => (isAnimating ? done() : enter(el, done))
                    "
                    @leave="
                        (el, done) => (isAnimating ? done() : leave(el, done))
                    "
                >
                    <div
                        v-for="file in calculatedFiles"
                        :key="file.id"
                        op0
                        class="fileCard"
                    >
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
import { useFuse } from '@vueuse/integrations/useFuse';

const files = useFiles();
const folders = useFolders();

const router = useRouter();

const searchQuery = ref('');
const currentPage = ref(1);
const filterType = ref([]);

const { results } = useFuse(searchQuery, files, {
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
});

const filtered = computed<FileData[]>(() =>
    results.value
        .map((r) => r.item)
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
const isLoading = ref(true);

const { all, enter, leave } = animateCards();

onMounted(async () => {
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

    all('files', '.fileCard');
});

watch(currentPage, () => {
    isAnimating.value = true;
    nextTick(() => {
        all('files', '.fileCard', () => {
            isAnimating.value = false;
        });
    });
});

definePageMeta({
    layout: 'dashboard',
    middleware: 'file-uploader-only',
});
</script>
