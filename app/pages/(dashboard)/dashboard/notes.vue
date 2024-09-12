<template>
    <div>
        <Head>
            <Title>Notes</Title>
        </Head>

        <ModalsTakeNotes v-model="takeNotesModalOpen" />

        <div space-y-6>
            <h2>Notes</h2>
            <UiSearchBar v-model="searchQuery" placeholder="Search notes..." />
            <div grid="~ gap6 lg:cols-3 md:cols-2 xl:cols-4 2xl:cols-5">
                <New h100px @action="takeNotesModalOpen = true" />

                <template v-if="isLoading">
                    <UiSkeletonCard
                        v-for="i in randomNumber(3, 7)"
                        :key="i"
                        h100px
                        flex="~ col gap4 justify-between"
                    >
                        <UiSkeletonLine
                            h5
                            :style="{
                                width: `${randomNumber(30, 70)}%`,
                            }"
                        />
                        <UiSkeletonLine h4 w40 text-slate300 />
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
                        v-for="note in calculatedNotes"
                        :key="note.id"
                        op0
                        class="noteCard"
                    >
                        <NoteCard :data="note" />
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

const notes = useNotes();

const searchQuery = ref('');
const currentPage = ref(1);

const takeNotesModalOpen = ref(false);

const { results } = useFuse(searchQuery, notes, {
    matchAllWhenSearchEmpty: true,
    fuseOptions: {
        keys: ['title'],
    },
});

const calculatedNotes = computed<NoteData[]>(() => {
    const start = (currentPage.value - 1) * 19;
    const end = start + 19;
    return results.value.map((r) => r.item).slice(start, end);
});

const isAnimating = ref(false);
const isLoading = ref(true);

const { all, enter, leave } = animateCards();

onMounted(async () => {
    const data = await $fetch('/api/notes');

    notes.value = data.map((n) => ({
        ...n,
        createdAt: new Date(n.createdAt),
    }));

    isLoading.value = false;
    await nextTick();

    all('notes', '.noteCard');
});

watch(currentPage, () => {
    isAnimating.value = true;
    nextTick(() => {
        all('notes', '.noteCard', () => {
            isAnimating.value = false;
        });
    });
});

definePageMeta({
    layout: 'dashboard',
    middleware: 'only-note-taker',
});
</script>
