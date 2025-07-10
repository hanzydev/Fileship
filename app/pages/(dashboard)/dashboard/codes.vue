<template>
    <div>
        <Head>
            <Title>Codes</Title>
        </Head>

        <ModalsShareCode v-model="shareCodeModalOpen" />

        <div space-y-6>
            <h2>Codes</h2>
            <UiSearchBar
                v-model="searchQuery"
                v-model:loading="isSearching"
                placeholder="Search codes..."
            />
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
                    <div v-for="code in calculatedCodes" :key="code.id" op0 class="codeCard">
                        <CodeCard :data="code" />
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
const codes = useCodes();
const currentUser = useAuthUser();

const currentPage = ref(1);
const searchQuery = ref('');
const searched = ref<string[]>([]);

const shareCodeModalOpen = ref(false);

const filtered = computed(() =>
    codes.value.filter((c) =>
        !isSearching.value && searchQuery.value.length ? searched.value.includes(c.id) : true,
    ),
);

const calculatedCodes = computed(() => {
    const start = (currentPage.value - 1) * 19;
    const end = start + 19;
    return filtered.value.slice(start, end);
});

const isAnimating = ref(false);
const isSearching = ref(false);
const isLoading = ref(codes.value.length !== currentUser.value!.stats.codes);

const { all, enter, leave } = animateCards();

onMounted(async () => {
    if (isLoading.value) {
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

let searchTimeout: NodeJS.Timeout;

watch(currentPage, () => {
    isAnimating.value = true;

    nextTick(() => {
        all('codes', '.codeCard', () => {
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
                searched.value = await $fetch<string[]>('/api/codes/search', {
                    method: 'POST',
                    body: { query },
                });
            } catch {
                searched.value = [];
            }

            isSearching.value = false;

            nextTick(() => {
                all('codes', '.codeCard', () => {
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
    middleware: 'code-sharer-only',
});
</script>
