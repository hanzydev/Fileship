<template>
    <Head>
        <Title>Notes</Title>
    </Head>

    <ModalsTakeNotes v-model="takeNotesModalOpen" />

    <DashboardContent>
        <template #header>
            <h2 lt-md="text-2xl!">Notes</h2>
        </template>

        <UiSearchBar
            v-model="searchQuery"
            v-model:loading="isSearching"
            placeholder="Search notes..."
        />
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
                    <UiSkeletonLine h4 w40 text-fs-muted-2 />
                </UiSkeletonCard>
            </template>
            <TransitionGroup
                v-else
                :css="false"
                @enter="(el, done) => (isAnimating ? done() : enter(el, done))"
                @leave="(el, done) => (isAnimating ? done() : leave(el, done))"
            >
                <div v-for="note in calculatedNotes" :key="note.id" op0 class="noteCard">
                    <NoteCard :data="note" />
                </div>
            </TransitionGroup>
        </div>
        <UiPagination v-model="currentPage" :item-count="filtered.length" :items-per-page="19" />
    </DashboardContent>
</template>

<script setup lang="ts">
const notes = useNotes();
const currentUser = useAuthUser();

const currentPage = ref(1);
const searchQuery = ref('');
const searched = ref<string[]>([]);

const takeNotesModalOpen = ref(false);

const filtered = computed(() =>
    notes.value.filter((n) =>
        !isSearching.value && searchQuery.value.length ? searched.value.includes(n.id) : true,
    ),
);

const calculatedNotes = computed(() => {
    const start = (currentPage.value - 1) * 19;
    const end = start + 19;
    return filtered.value.slice(start, end);
});

const isAnimating = ref(false);
const isSearching = ref(false);
const isLoading = ref(notes.value.length !== currentUser.value!.stats.notes);

const { all, enter, leave } = animateCards();

onMounted(async () => {
    if (isLoading.value) {
        const data = await $fetch('/api/notes');

        notes.value = data.map((n) => ({
            ...n,
            createdAt: new Date(n.createdAt),
        }));

        isLoading.value = false;
        await nextTick();
    }

    all('notes', '.noteCard');
});

let searchTimeout: NodeJS.Timeout;

watch(currentPage, () => {
    isAnimating.value = true;
    nextTick(() => {
        all('notes', '.noteCard', () => {
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
                searched.value = await $fetch<string[]>('/api/notes/search', {
                    method: 'POST',
                    body: { query },
                });
            } catch {
                searched.value = [];
            }

            isSearching.value = false;

            nextTick(() => {
                all('notes', '.noteCard', () => {
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
    middleware: 'note-taker-only',
});
</script>
