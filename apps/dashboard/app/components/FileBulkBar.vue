<template>
    <Transition
        enter-active-class="animate-in fade-in slide-in-from-bottom-4 duration-200"
        leave-active-class="animate-out fade-out slide-out-to-bottom-4 duration-200"
    >
        <div
            v-if="selected.length"
            class="fixed bottom-4 left-1/2 z-30 flex -translate-x-1/2 items-center gap-1 rounded-full border bg-popover p-1.5 pl-4 shadow-lg"
        >
            <TextBlock variant="small" class="mr-2 whitespace-nowrap">
                {{ selected.length }} selected
            </TextBlock>
            <UiButton
                variant="ghost"
                size="sm"
                class="hidden sm:flex"
                :disabled="isBusy"
                @click="handleSelectAll"
            >
                {{ isAllSelected ? 'Deselect all' : 'Select all' }}
            </UiButton>
            <UiButton
                variant="ghost"
                size="sm"
                @click="moveTargets = selected.map((id) => ({ id, folderId: null }))"
            >
                <IFolderInput />
                <span class="hidden sm:inline">Move</span>
            </UiButton>
            <UiButton variant="ghost" size="sm" :disabled="isBusy" @click="handleDownload">
                <IDownload />
                <span class="hidden sm:inline">Download</span>
            </UiButton>
            <UiButton variant="ghost" size="sm" @click="handleDelete">
                <ITrash2 />
                <span class="hidden sm:inline">Delete</span>
            </UiButton>
            <UiButton variant="ghost" size="icon-sm" @click="selected = []">
                <IX />
            </UiButton>
        </div>
    </Transition>

    <FileMoveDialog v-model="moveTargets" @moved="selected = []" />
</template>

<script setup lang="ts">
const { items, total, query } = defineProps<{
    items: FileData[];
    total: number;
    query?: Record<string, unknown>;
}>();

const selected = defineModel<string[]>({ required: true });

const { $toast } = useNuxtApp();
const { download, remove } = useFileActions();

const isBusy = ref(false);
const moveTargets = ref<Pick<FileData, 'id' | 'folderId'>[]>([]);

const isAllSelected = computed(() => selected.value.length >= total);

const handleSelectAll = async () => {
    if (isAllSelected.value) {
        selected.value = [];
        return;
    }

    if (!query) {
        selected.value = items.map((file) => file.id);
        return;
    }

    isBusy.value = true;

    try {
        selected.value = await $fetch('/api/files/ids', { query });
    } catch (error: any) {
        $toast.error(error.data.message);
    }

    isBusy.value = false;
};

const handleDownload = async () => {
    const loaded = items.filter((file) => selected.value.includes(file.id));
    const missing = selected.value.filter((id) => !loaded.some((file) => file.id === id));

    isBusy.value = true;

    try {
        const chunks = Array.from({ length: Math.ceil(missing.length / 100) }, (_, index) =>
            missing.slice(index * 100, index * 100 + 100),
        );

        const pages = await Promise.all(
            chunks.map((ids) =>
                $fetch('/api/files', { query: { ids: ids.join(','), limit: ids.length } }),
            ),
        );

        download([...loaded, ...(pages.flatMap((page) => page.items) as never[])]);
    } catch (error: any) {
        $toast.error(error.data.message);
    }

    isBusy.value = false;
};

const handleDelete = async () => {
    const files = selected.value.map(
        (id) => items.find((file) => file.id === id) ?? { id, fileName: 'file' },
    );

    if (await remove(files)) selected.value = [];
};
</script>
