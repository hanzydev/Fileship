<template>
    <div>
        <Head>
            <Title>Folders</Title>
        </Head>

        <ModalsCreateFolder v-model="createFolderModalOpen" />

        <div space-y-6>
            <h2>Folders</h2>
            <UiSearchBar
                v-model="searchQuery"
                placeholder="Search folders..."
            />
            <div grid="~ gap6 lg:cols-3 md:cols-2 xl:cols-4 2xl:cols-5">
                <New h164px @action="createFolderModalOpen = true" />

                <TransitionGroup
                    :css="false"
                    @enter="!isAnimating && enter"
                    @leave="!isAnimating && leave"
                >
                    <div
                        v-for="folder in calculatedFolders"
                        :key="folder.id"
                        opacity-0
                        class="folderCard"
                    >
                        <FolderCard :data="folder" />
                    </div>
                </TransitionGroup>
            </div>
            <UiPagination
                v-model="currentPage"
                :item-count="results.length"
                :items-per-page="19"
            />
        </div>
    </div>
</template>

<script setup lang="ts">
import { useFuse } from '@vueuse/integrations/useFuse';

const folders = useFolders();
const files = useFiles();

const searchQuery = ref('');
const currentPage = ref(1);

const createFolderModalOpen = ref(false);

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

const { results } = useFuse(searchQuery, folders, {
    matchAllWhenSearchEmpty: true,
    fuseOptions: {
        keys: ['name'],
    },
});

const calculatedFolders = computed<FolderData[]>(() => {
    const start = (currentPage.value - 1) * 19;
    const end = start + 19;
    return results.value.map((r) => r.item).slice(start, end);
});

const isAnimating = ref(false);
const { all, enter, leave } = animateCards();

onMounted(() => all('folders', '.folderCard'));

watch(
    currentPage,
    () => {
        isAnimating.value = true;
        all('folders', '.folderCard', () => {
            isAnimating.value = false;
        });
    },
    { flush: 'post' },
);

definePageMeta({
    layout: 'dashboard',
    middleware: 'only-file-uploader',
});
</script>
