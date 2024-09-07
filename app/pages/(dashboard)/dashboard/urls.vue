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

                <TransitionGroup
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
                        opacity-0
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

const { data } = await useFetch('/api/urls');

urls.value = data.value!.map((u) => ({
    ...u,
    expiresAt: u.expiresAt ? new Date(u.expiresAt) : null,
    createdAt: new Date(u.createdAt),
}));

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
const { all, enter, leave } = animateCards();

onMounted(() => all('urls', '.urlCard'));

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
    middleware: 'only-url-shortener',
});
</script>
