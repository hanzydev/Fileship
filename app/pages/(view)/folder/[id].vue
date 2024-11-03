<template>
    <div>
        <Head>
            <Title>{{ data!.name }}</Title>
        </Head>

        <div hfull min-hscreen bg-fs-overlay-1 p8="!">
            <div space-y-6>
                <h2>{{ data!.name }}</h2>

                <UiSearchBar v-model="searchQuery" placeholder="Search files..." />
                <div v-if="results.length" grid="~ gap6 lg:cols-3 md:cols-2 xl:cols-4">
                    <template v-for="file in calculatedFiles" :key="file.id">
                        <PartialFileCard
                            :data="{
                                ...file,
                                createdAt: new Date(file.createdAt),
                                embed: data!.embed,
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
                    :item-count="results.length"
                    :items-per-page="20"
                />
            </div>
        </div>
    </div>
</template>

<script setup lang="ts">
import { useFuse } from '@vueuse/integrations/useFuse';

const route = useRoute();

const { data, error } = await useFetch(`/api/folders/${route.params.id}`, {
    headers: useRequestHeaders(['host']),
});

if (error.value) {
    throw createError({
        statusCode: 404,
        statusMessage: 'Not Found',
        message: 'Folder not found',
    });
}

const searchQuery = ref('');
const currentPage = ref(1);

const { results } = useFuse(searchQuery, data.value!.files, {
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

const calculatedFiles = computed<FileData[]>(() => {
    const start = (currentPage.value - 1) * 20;
    const end = start + 20;
    return results.value.map((r) => r.item).slice(start, end) as never;
});
</script>
