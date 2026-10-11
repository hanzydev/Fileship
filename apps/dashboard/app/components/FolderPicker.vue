<template>
    <UiPopover v-model:open="isOpen">
        <UiPopoverTrigger as-child>
            <UiButton variant="outline" class="w-full justify-between" :disabled="disabled">
                <span class="flex items-center gap-2 truncate">
                    <IFolder />
                    {{ (folder && selectedName) || 'No folder' }}
                </span>
                <IChevronsUpDown class="opacity-50" />
            </UiButton>
        </UiPopoverTrigger>
        <UiPopoverContent class="w-(--reka-popover-trigger-width) p-0" align="start">
            <UiCommand>
                <UiCommandInput
                    placeholder="Search or create a folder..."
                    @update:model-value="query = $event"
                />
                <UiCommandList class="mb-1" @scroll="handleScroll">
                    <UiCommandGroup class="pb-0">
                        <UiCommandItem
                            value="none"
                            :data-checked="!folder"
                            @select="handleSelect(null)"
                        >
                            <IFolderX />
                            No folder
                        </UiCommandItem>
                        <UiCommandItem
                            v-for="f in folders"
                            :key="f.id"
                            :value="f.id"
                            :data-checked="folder === f.id"
                            @select="handleSelect(f)"
                        >
                            <IFolder />
                            <span class="truncate">{{ f.name }}</span>
                        </UiCommandItem>
                    </UiCommandGroup>
                    <UiCommandGroup v-if="canCreate" class="pb-0">
                        <UiCommandItem
                            :key="query"
                            :value="`create:${query}`"
                            :disabled="isCreating"
                            @select="handleCreate"
                        >
                            <UiSpinner v-if="isCreating" />
                            <IFolderPlus v-else />
                            Create "{{ query.trim() }}"
                        </UiCommandItem>
                    </UiCommandGroup>
                </UiCommandList>
            </UiCommand>
        </UiPopoverContent>
    </UiPopover>
</template>

<script setup lang="ts">
const { disabled } = defineProps<{
    disabled?: boolean;
}>();

const folder = defineModel<string | null>({ required: true });

const { $toast } = useNuxtApp();

const query = ref('');
const isOpen = ref(false);
const isCreating = ref(false);
const selectedName = ref<string>();

const { items: folders, loadMore } = usePaginatedList<FolderData>(
    'folders',
    '/api/folders',
    () => ({ q: query.value.trim() }),
    { limit: 30, enabled: isOpen },
);

const canCreate = computed(
    () =>
        !!query.value.trim() &&
        !folders.value.some((f) => f.name.toLowerCase() === query.value.trim().toLowerCase()),
);

const handleSelect = (selected: { id: string; name: string } | null) => {
    selectedName.value = selected?.name;
    folder.value = selected?.id ?? null;
    isOpen.value = false;
};

const handleScroll = (event: Event) => {
    const target = event.target as HTMLElement;
    if (target.scrollHeight - target.scrollTop - target.clientHeight < 80) loadMore();
};

const handleCreate = async () => {
    isCreating.value = true;

    try {
        const created = await $fetch('/api/folders', {
            method: 'POST',
            body: { name: query.value.trim() },
        });

        handleSelect(created);
        $toast.success('Folder created and selected');
    } catch (error: any) {
        $toast.error(error.data.data?.name?._errors[0] || error.data.message);
    }

    isCreating.value = false;
};

watch(
    folder,
    async (value) => {
        if (!value) return;

        selectedName.value =
            folders.value.find((f) => f.id === value)?.name ??
            (await $fetch(`/api/folders/${value}`).catch(() => null))?.name;
    },
    { immediate: true },
);

watch(isOpen, (value) => value && (query.value = ''));
</script>
