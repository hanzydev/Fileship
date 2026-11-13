<template>
    <UiCommandDialog
        v-model:open="isOpen"
        class="top-1/2 -translate-y-1/2!"
        :title="$t('docs.search.title')"
        :description="$t('docs.search.description')"
    >
        <UiCommandInput
            :placeholder="$t('docs.search.placeholder')"
            @update:model-value="query = String($event ?? '')"
        />
        <UiCommandList class="mb-1 max-h-[min(60vh,28rem)]">
            <UiCommandEmpty>{{ $t('docs.search.empty') }}</UiCommandEmpty>
            <UiCommandGroup v-if="results.length" class="px-1 pb-0">
                <UiCommandItem
                    v-for="result in results"
                    :key="`${result.id}:${term}`"
                    :value="result.id"
                    class="items-start"
                    @select="handleSelect(result.id)"
                >
                    <IHash v-if="result.level > 1" class="mt-0.5" />
                    <IFileText v-else class="mt-0.5" />
                    <div class="flex min-w-0 flex-col gap-0.5">
                        <span class="truncate">{{ result.title }}</span>
                        <span v-if="result.path" class="truncate text-xs text-muted-foreground">
                            {{ result.path }}
                        </span>
                        <span
                            v-if="result.excerpt"
                            class="line-clamp-2 text-xs text-muted-foreground"
                        >
                            {{ result.excerpt }}
                        </span>
                    </div>
                </UiCommandItem>
            </UiCommandGroup>
        </UiCommandList>
    </UiCommandDialog>
</template>

<script setup lang="ts">
const LIMIT = 40;

const isOpen = useState('docs-search-open', () => false);
const query = ref('');

const term = computed(() => query.value.trim().toLocaleLowerCase());

const { version } = useDocsVersion();

const { data, execute } = useLazyAsyncData(
    () => `docs-search-${version.value.collection}`,
    () => queryCollectionSearchSections(version.value.collection),
    { server: false, immediate: false },
);

watch(isOpen, (open) => {
    if (!open) return;
    query.value = '';
    if (!data.value) execute();
});

const sections = computed(() => {
    const seen = new Set<string>();

    return (data.value ?? [])
        .map((section) => ({
            id: section.id,
            level: section.level,
            title: section.title,
            path: section.titles.join(' › '),
            content: section.content.replace(/\s+/g, ' ').trim(),
        }))
        .filter((section) => {
            const key = `${section.path}|${section.title}`;
            if (seen.has(key)) return false;
            seen.add(key);
            return true;
        });
});

const excerptOf = (content: string, index: number) => {
    const start = Math.max(0, index - 50);
    const end = Math.min(content.length, index + term.value.length + 90);

    return `${start ? '…' : ''}${content.slice(start, end)}${end < content.length ? '…' : ''}`;
};

const results = computed(() => {
    if (!term.value)
        return sections.value
            .filter((section) => section.level === 1)
            .map((section) => ({ ...section, excerpt: '' }));

    const titled = [];
    const mentioned = [];

    for (const section of sections.value) {
        const inTitle = `${section.path} ${section.title}`.toLocaleLowerCase().includes(term.value);
        const index = section.content.toLocaleLowerCase().indexOf(term.value);

        if (inTitle) titled.push({ ...section, excerpt: '' });
        else if (index >= 0)
            mentioned.push({ ...section, excerpt: excerptOf(section.content, index) });
    }

    return [...titled, ...mentioned].slice(0, LIMIT);
});

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
