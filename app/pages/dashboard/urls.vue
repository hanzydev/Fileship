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
                <UrlCard
                    v-for="url in calculatedUrls"
                    :key="url.id"
                    :data="url"
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

const filtered = computed(() =>
    urls.value.filter((u) =>
        Object.values(u).some((v) =>
            v
                ?.toString()
                ?.toLowerCase()
                ?.includes(searchQuery.value.toLowerCase()),
        ),
    ),
);

const calculatedUrls = computed(() => {
    const start = (currentPage.value - 1) * 19;
    const end = start + 19;
    return filtered.value.slice(start, end);
});

definePageMeta({
    layout: 'dashboard',
    middleware: 'only-url-shortener',
});
</script>
