<template>
    <div>
        <Head>
            <Title>URLs</Title>
        </Head>

        <ModalsShortenUrl v-model="shortenUrlModalOpen" />

        <div space-y-6>
            <h2>URLs</h2>
            <UiSearchBar
                v-model="searchQuery"
                v-model:loading="isSearching"
                placeholder="Search urls..."
            />
            <div grid="~ gap6 lg:cols-3 md:cols-2 xl:cols-4 2xl:cols-5">
                <New h132px @action="shortenUrlModalOpen = true" />

                <template v-if="isLoading">
                    <UiSkeletonCard
                        v-for="i in randomNumber(3, 7)"
                        :key="i"
                        h132px
                        flex="~ col gap4 justify-between"
                    >
                        <UiSkeletonLine
                            h5
                            :style="{
                                width: `${randomNumber(30, 70)}%`,
                            }"
                        />
                        <div text-fs-muted-2 space-y-2>
                            <UiSkeletonLine h4 w16 />
                            <UiSkeletonLine h4 w40 />
                        </div>
                    </UiSkeletonCard>
                </template>
                <TransitionGroup
                    v-else
                    :css="false"
                    @enter="(el, done) => (isAnimating ? done() : enter(el, done))"
                    @leave="(el, done) => (isAnimating ? done() : leave(el, done))"
                >
                    <div v-for="url in calculatedUrls" :key="url.id" op0 class="urlCard">
                        <UrlCard :data="url" />
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
const urls = useUrls();
const currentUser = useAuthUser();

const currentPage = ref(1);
const searchQuery = ref('');
const searched = ref<string[]>([]);

const shortenUrlModalOpen = ref(false);

const filtered = computed(() =>
    urls.value.filter((u) =>
        !isSearching.value && searchQuery.value.length ? searched.value.includes(u.id) : true,
    ),
);

const calculatedUrls = computed(() => {
    const start = (currentPage.value - 1) * 19;
    const end = start + 19;
    return filtered.value.slice(start, end);
});

const isAnimating = ref(false);
const isSearching = ref(false);
const isLoading = ref(urls.value.length !== currentUser.value!.stats.urls);

const { all, enter, leave } = animateCards();

onMounted(async () => {
    if (isLoading.value) {
        const data = await $fetch('/api/urls');

        urls.value = data.map((u) => ({
            ...u,
            expiresAt: u.expiresAt ? new Date(u.expiresAt) : null,
            createdAt: new Date(u.createdAt),
        }));

        isLoading.value = false;
        await nextTick();
    }

    all('urls', '.urlCard');
});

let searchTimeout: NodeJS.Timeout;

watch(currentPage, () => {
    isAnimating.value = true;
    nextTick(() => {
        all('urls', '.urlCard', () => {
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
                searched.value = await $fetch<string[]>('/api/urls/search', {
                    method: 'POST',
                    body: { query },
                });
            } catch {
                searched.value = [];
            }

            isSearching.value = false;

            nextTick(() => {
                all('urls', '.urlCard', () => {
                    isAnimating.value = false;
                });
            });
        }, 750);
    } else {
        searched.value = [];
    }
});

definePageMeta({
    layout: 'dashboard',
    middleware: 'url-shortener-only',
});
</script>
