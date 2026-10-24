<template>
    <UiPopover v-model:open="isOpen">
        <UiPopoverTrigger as-child>
            <UiButton variant="outline" class="w-full justify-between" :disabled="disabled">
                <span class="flex items-center gap-2 truncate">
                    <IFolder />
                    {{ (folder && selectedName) || noneLabel || $t('folders.select.none') }}
                </span>
                <IChevronsUpDown class="opacity-50" />
            </UiButton>
        </UiPopoverTrigger>
        <UiPopoverContent class="w-(--reka-popover-trigger-width) p-0" align="start">
            <UiCommand>
                <UiCommandInput
                    :placeholder="$t('folders.select.searchPlaceholder')"
                    @update:model-value="query = $event"
                />
                <UiCommandList class="mb-1">
                    <UiCommandGroup class="px-1 pb-0">
                        <UiCommandItem
                            value="none"
                            :data-checked="!folder"
                            @select="handleSelect(null)"
                        >
                            <IFolderX />
                            {{ noneLabel || $t('folders.select.none') }}
                        </UiCommandItem>
                        <VirtualList
                            :items="choices"
                            :item-key="(f) => f.id"
                            :estimate-size="36"
                            :gap="0"
                            contained
                            @reach-end="loadMore"
                        >
                            <template #default="{ item }">
                                <UiCommandItem
                                    :value="item.id"
                                    :data-checked="folder === item.id"
                                    @select="handleSelect(item)"
                                >
                                    <IFolder />
                                    <span class="truncate">{{ item.name }}</span>
                                    <span
                                        v-if="item.parent"
                                        class="ml-auto max-w-1/2 shrink-0 truncate text-xs text-muted-foreground"
                                    >
                                        {{ item.parent.name }}
                                    </span>
                                </UiCommandItem>
                            </template>
                        </VirtualList>
                    </UiCommandGroup>
                    <UiCommandGroup v-if="canCreate" class="px-1 pb-0">
                        <UiCommandItem
                            :key="query"
                            :value="`create:${query}`"
                            :disabled="isCreating"
                            @select="handleCreate"
                        >
                            <UiSpinner v-if="isCreating" />
                            <IFolderPlus v-else />
                            {{ $t('folders.select.create', { name: query.trim() }) }}
                        </UiCommandItem>
                    </UiCommandGroup>
                </UiCommandList>
            </UiCommand>
        </UiPopoverContent>
    </UiPopover>
</template>

<script setup lang="ts">
import type { FolderData } from '~/types/folders';

const { disabled, exclude, noneLabel } = defineProps<{
    disabled?: boolean;
    exclude?: string;
    noneLabel?: string;
}>();

const folder = defineModel<string | null>({ required: true });

const { $toast } = useNuxtApp();
const { t } = useI18n();

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

const choices = computed(() =>
    exclude
        ? folders.value.filter((f) => f.id !== exclude && f.parent?.id !== exclude)
        : folders.value,
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

const handleCreate = async () => {
    isCreating.value = true;

    try {
        const created = await $fetch('/api/folders', {
            method: 'POST',
            body: { name: query.value.trim() },
        });

        handleSelect(created);
        $toast.success(t('folders.select.created'));
    } catch (error: any) {
        $toast.error(error.data?.data?.formErrors?.name?._errors[0] || getErrorMessage(error));
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
