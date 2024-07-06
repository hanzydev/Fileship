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
                <NoteCard
                    v-for="note in calculatedNotes"
                    :key="note.id"
                    :data="note"
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
const notes = useNotes();

const searchQuery = ref('');
const currentPage = ref(1);

const takeNotesModalOpen = ref(false);

const { data } = await useFetch('/api/notes');

notes.value = data.value!.map((n) => ({
    ...n,
    createdAt: new Date(n.createdAt),
}));

const filtered = computed(() =>
    notes.value.filter((n) =>
        Object.values(n).some((v) =>
            v
                ?.toString()
                ?.toLowerCase()
                ?.includes(searchQuery.value.toLowerCase()),
        ),
    ),
);

const calculatedNotes = computed(() => {
    const start = (currentPage.value - 1) * 19;
    const end = start + 19;
    return filtered.value.slice(start, end);
});

definePageMeta({
    layout: 'dashboard',
    middleware: 'only-note-taker',
});
</script>
