<template>
    <div>
        <Head>
            <Title>{{ data!.name }}</Title>
        </Head>

        <div hfull min-hscreen bg-fs-overlay-1 p8="!">
            <div space-y-6>
                <h2>{{ data!.name }}</h2>

                <UiSearchBar
                    v-model="searchQuery"
                    placeholder="Search files..."
                />
                <div
                    v-if="filtered.length"
                    grid="~ gap6 lg:cols-3 md:cols-2 xl:cols-4"
                >
                    <template v-for="file in calculatedFiles" :key="file.id">
                        <PartialFileCard
                            :data="{
                                ...file,
                                createdAt: new Date(file.createdAt),
                            }"
                        />
                    </template>
                </div>
                <NothingHere
                    v-else
                    message="There are no files to display."
                    icon="heroicons-solid:document-duplicate"
                />
                <UiPagination
                    v-model="currentPage"
                    :item-count="filtered.length"
                    :items-per-page="20"
                />
            </div>
        </div>
    </div>
</template>

<script setup lang="ts">
const route = useRoute();

const { data, error } = await useFetch(`/api/folders/${route.params.id}`);

if (error.value) {
    throw createError({
        statusCode: 404,
        statusMessage: 'Not Found',
        message: 'Folder not found',
    });
}

const searchQuery = ref('');
const currentPage = ref(1);

const filtered = computed(() =>
    data.value!.files.filter((f) =>
        Object.values(f).some((v) =>
            v
                ?.toString()
                ?.toLowerCase()
                ?.includes(searchQuery.value.toLowerCase()),
        ),
    ),
);

const calculatedFiles = computed(() => {
    const start = (currentPage.value - 1) * 20;
    const end = start + 20;
    return filtered.value.slice(start, end);
});
</script>
