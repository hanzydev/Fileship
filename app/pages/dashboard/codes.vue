<template>
    <div>
        <Head>
            <Title>Codes</Title>
        </Head>

        <ModalsShareCode v-model="shareCodeModalOpen" />

        <div space-y-6>
            <h2>Codes</h2>
            <UiSearchBar v-model="searchQuery" placeholder="Search codes..." />
            <div grid="~ gap6 lg:cols-3 md:cols-2 xl:cols-4 2xl:cols-5">
                <New h132px @action="shareCodeModalOpen = true" />
                <CodeCard
                    v-for="code in calculatedCodes"
                    :key="code.id"
                    :data="code"
                />
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

const codes = useCodes();

const searchQuery = ref('');
const currentPage = ref(1);

const shareCodeModalOpen = ref(false);

const { data } = await useFetch('/api/codes');

codes.value = data.value!.map((c) => ({
    ...c,
    expiresAt: c.expiresAt ? new Date(c.expiresAt) : null,
    createdAt: new Date(c.createdAt),
}));

const { results } = useFuse(searchQuery, codes, {
    matchAllWhenSearchEmpty: true,
    fuseOptions: {
        keys: [
            {
                name: 'title',
                weight: 2,
            },
            'language',
        ],
    },
});

const calculatedCodes = computed<CodeData[]>(() => {
    const start = (currentPage.value - 1) * 19;
    const end = start + 19;
    return results.value.map((r) => r.item).slice(start, end);
});

definePageMeta({
    layout: 'dashboard',
    middleware: 'only-code-sharer',
});
</script>
