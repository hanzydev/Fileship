<template>
    <div>
        <Head>
            <Title>{{ data!.name }}</Title>
        </Head>

        <div hfull min-hscreen bg-fs-overlay-1 p8="!">
            <div space-y-6>
                <h2>{{ data!.name }}</h2>

                <div flex="~ gap4 1 items-center" wfull>
                    <UiSearchBar v-model="searchQuery" placeholder="Search files..." wfull />
                    <FileTypeFilter v-model="filterType" />
                </div>

                <div v-show="filtered.length" grid="~ gap6 md:cols-2 lg:cols-3 xl:cols-4">
                    <TransitionGroup
                        :css="false"
                        @enter="(el, done) => (isAnimating ? done() : enter(el, done))"
                        @leave="(el, done) => (isAnimating ? done() : leave(el, done))"
                    >
                        <div v-for="file in calculatedFiles" :key="file.id" op0 class="fileCard">
                            <PartialFileCard
                                :data="{
                                    ...file,
                                    createdAt: new Date(file.createdAt),
                                    embed: data!.embed,
                                }"
                            />
                        </div>
                    </TransitionGroup>
                </div>
                <NothingHere
                    v-if="!filtered.length"
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
const filterType = ref([]);

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

const filtered = computed(() =>
    results.value
        .map((r) => r.item)
        .filter(
            (f) =>
                !filterType.value.length ||
                filterType.value.some((t) => f.mimeType.startsWith(`${t}/`)),
        ),
);

const calculatedFiles = computed(() => {
    const start = (currentPage.value - 1) * 20;
    const end = start + 20;
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
</script>
