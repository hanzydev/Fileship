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
                <TransitionGroup
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
                        opacity-0
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

const { data: foldersData } = await useFetch('/api/folders');
const { data: filesData } = await useFetch('/api/files');

folders.value = foldersData.value!.map((f) => ({
    ...f,
    createdAt: new Date(f.createdAt),
}));

files.value = filesData.value!.map((f) => ({
    ...f,
    expiresAt: f.expiresAt ? new Date(f.expiresAt) : null,
    createdAt: new Date(f.createdAt),
}));

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
const { all, enter, leave } = animateCards();

onMounted(() => all('files', '.fileCard'));

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
    middleware: 'only-file-uploader',
});
</script>
