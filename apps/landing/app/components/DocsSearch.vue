<template>
    <UiCommandDialog
        v-model:open="isOpen"
        :title="$t('docs.search.title')"
        :description="$t('docs.search.description')"
    >
        <UiCommandInput :placeholder="$t('docs.search.placeholder')" />
        <UiCommandList class="mb-1 max-h-[60vh]">
            <UiCommandEmpty>{{ $t('docs.search.empty') }}</UiCommandEmpty>
            <UiCommandGroup class="px-1 pb-0">
                <UiCommandItem
                    v-for="section in sections"
                    :key="section.id"
                    :value="section.value"
                    class="items-start"
                    @select="handleSelect(section.id)"
                >
                    <IHash v-if="section.level > 1" class="mt-0.5" />
                    <IFileText v-else class="mt-0.5" />
                    <div class="flex min-w-0 flex-col gap-0.5">
                        <span class="truncate">{{ section.title }}</span>
                        <span v-if="section.path" class="truncate text-xs text-muted-foreground">
                            {{ section.path }}
                        </span>
                    </div>
                </UiCommandItem>
            </UiCommandGroup>
        </UiCommandList>
    </UiCommandDialog>
</template>

<script setup lang="ts">
const isOpen = useState('docs-search-open', () => false);

const { data, execute } = useLazyAsyncData(
    'docs-search',
    () => queryCollectionSearchSections('docs'),
    { server: false, immediate: false },
);

watch(isOpen, (open) => open && !data.value && execute());

const sections = computed(() =>
    (data.value ?? []).map((section) => ({
        id: section.id,
        level: section.level,
        title: section.title,
        path: section.titles.join(' › '),
        value: [section.id, ...section.titles, section.title, section.content.slice(0, 160)].join(
            ' ',
        ),
    })),
);

const handleSelect = (id: string) => {
    isOpen.value = false;
    navigateTo(id);
};

onKeyStroke(['k', 'K'], (event) => {
    if (event.ctrlKey || event.metaKey) {
        event.preventDefault();
        isOpen.value = !isOpen.value;
    }
});
</script>
