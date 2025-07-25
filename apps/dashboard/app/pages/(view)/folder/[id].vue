<template>
    <div>
        <Head>
            <Title>{{ data!.name }}</Title>
        </Head>

        <div hfull min-hscreen bg-fs-overlay-1 p8="!">
            <div space-y-6>
                <h2>{{ data!.name }}</h2>

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

                <div v-show="filtered.length" grid="~ gap6 md:cols-2 lg:cols-3 xl:cols-4">
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
                    :item-count="filtered.length"
                    :items-per-page="20"
                />
            </div>
        </div>
    </div>
</template>

<script setup lang="ts">
const route = useRoute();

const { data, error } = await useFetch<FolderData & { files: FileData[]; embed: IEmbed }>(
    `/api/folders/${route.params.id}`,
    {
        headers: useRequestHeaders(['host']),
    },
);

if (error.value) {
    throw createError({
        statusCode: 404,
        message: 'Folder not found',
    });
}

const currentPage = ref(1);
const filterType = ref([]);
const searchQuery = ref('');
const aiEnabled = ref(false);
const searched = ref<string[]>([]);

useFiles().value = data.value!.files as any[];

const filtered = computed(() =>
    (data.value!.files as FileData[])
        .filter((f) =>
            !isSearching.value && searchQuery.value.length ? searched.value.includes(f.id) : true,
        )
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
const isSearching = ref(false);
const isLoading = ref(false);

const { all, enter, leave } = animateCards();

onMounted(() => all('files', '.fileCard'));

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
                searched.value = await $fetch<string[]>(
                    `/api/folders/${data.value!.id}/searchFiles`,
                    {
                        method: 'POST',
                        body: { query, mode: aiEnabled.value ? 'vector' : 'fulltext' },
                    },
                );
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
</script>
