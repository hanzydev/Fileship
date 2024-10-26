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
                        v-for="code in calculatedCodes"
                        :key="code.id"
                        op0
                        class="codeCard"
                    >
                        <CodeCard :data="code" />
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

const codes = useCodes();

const searchQuery = ref('');
const currentPage = ref(1);

const shareCodeModalOpen = ref(false);

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

const isAnimating = ref(false);
const isLoading = ref(!codes.value.length);

const { all, enter, leave } = animateCards();

onMounted(async () => {
    if (!codes.value.length) {
        const data = await $fetch('/api/codes');

        codes.value = data.map((c) => ({
            ...c,
            expiresAt: c.expiresAt ? new Date(c.expiresAt) : null,
            createdAt: new Date(c.createdAt),
        }));

        isLoading.value = false;
        await nextTick();
    }

    all('codes', '.codeCard');
});

watch(currentPage, () => {
    isAnimating.value = true;

    nextTick(() => {
        all('codes', '.codeCard', () => {
            isAnimating.value = false;
        });
    });
});

definePageMeta({
    layout: 'dashboard',
    middleware: 'code-sharer-only',
});
</script>
