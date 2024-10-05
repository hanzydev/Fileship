<template>
    <div>
        <Head>
            <Title>URLs</Title>
        </Head>

        <ModalsShortenUrl v-model="shortenUrlModalOpen" />

        <div space-y-6>
            <h2>URLs</h2>
            <UiSearchBar v-model="searchQuery" placeholder="Search urls..." />
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
                        <div text-slate300 space-y-2>
                            <UiSkeletonLine h4 w16 />
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
                        v-for="url in calculatedUrls"
                        :key="url.id"
                        op0
                        class="urlCard"
                    >
                        <UrlCard :data="url" />
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

const urls = useUrls();

const searchQuery = ref('');
const currentPage = ref(1);

const shortenUrlModalOpen = ref(false);

const { results } = useFuse(searchQuery, urls, {
    matchAllWhenSearchEmpty: true,
    fuseOptions: {
        keys: [
            {
                name: 'vanity',
                weight: 2,
            },
            'destinationUrl',
        ],
    },
});

const calculatedUrls = computed<UrlData[]>(() => {
    const start = (currentPage.value - 1) * 19;
    const end = start + 19;
    return results.value.map((r) => r.item).slice(start, end);
});

const isAnimating = ref(false);
const isLoading = ref(true);

const { all, enter, leave } = animateCards();

onMounted(async () => {
    const data = await $fetch('/api/urls');

    urls.value = data.map((u) => ({
        ...u,
        expiresAt: u.expiresAt ? new Date(u.expiresAt) : null,
        createdAt: new Date(u.createdAt),
    }));

    isLoading.value = false;
    await nextTick();

    all('urls', '.urlCard');
});

watch(currentPage, () => {
    isAnimating.value = true;
    nextTick(() => {
        all('urls', '.urlCard', () => {
            isAnimating.value = false;
        });
    });
});

definePageMeta({
    layout: 'dashboard',
    middleware: 'url-shortener-only',
});
</script>
