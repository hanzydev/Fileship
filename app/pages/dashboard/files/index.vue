<template>
    <div>
        <Head>
            <Title>Files</Title>
        </Head>

        <div space-y-6>
            <h2>Files</h2>
            <UiSearchBar v-model="searchQuery" placeholder="Search files..." />
            <div grid="~ gap6 lg:cols-3 md:cols-2 xl:cols-4">
                <New h208px @action="$router.push('/dashboard/files/upload')" />
                <FileCard
                    v-for="file in calculatedFiles"
                    :key="file.id"
                    :data="file"
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
const files = useFiles();
const folders = useFolders();

const searchQuery = ref('');
const currentPage = ref(1);

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
    files.value
        .filter((f) => !f.folderId)
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
    const start = (currentPage.value - 1) * 19;
    const end = start + 19;
    return filtered.value.slice(start, end);
});

definePageMeta({
    layout: 'dashboard',
    middleware: 'only-file-uploader',
});
</script>
