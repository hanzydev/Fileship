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
                        <div text-slate300 space-y-2>
                            <UiSkeletonLine h4 w16 />
                            <UiSkeletonLine h4 w14 />
                            <UiSkeletonLine h4 w40 />
                        </div>
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
                        v-for="folder in calculatedFolders"
                        :key="folder.id"
                        op0
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

    all('folders', '.folderCard');
});

watch(currentPage, () => {
    isAnimating.value = true;
    nextTick(() => {
        all('folders', '.folderCard', () => {
            isAnimating.value = false;
        });
    });
});

definePageMeta({
    layout: 'dashboard',
    middleware: 'file-uploader-only',
});
</script>
