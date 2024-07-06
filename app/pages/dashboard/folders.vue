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
                <FolderCard
                    v-for="folder in calculatedFolders"
                    :key="folder.id"
                    :data="folder"
                />
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

const filtered = computed(() =>
    folders.value.filter((f) =>
        Object.values(f).some((v) =>
            v
                ?.toString()
                ?.toLowerCase()
                ?.includes(searchQuery.value.toLowerCase()),
        ),
    ),
);

const calculatedFolders = computed(() => {
    const start = (currentPage.value - 1) * 19;
    const end = start + 19;
    return filtered.value.slice(start, end);
});

definePageMeta({
    layout: 'dashboard',
    middleware: 'only-file-uploader',
});
</script>
