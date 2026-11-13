<template>
    <UiSelect :model-value="version.key" @update:model-value="handleChange($event as string)">
        <UiSelectTrigger class="w-full" :aria-label="$t('docs.versions.label')">
            <UiSelectValue>
                {{ $t(`docs.versions.${version.key}`, { version: version.version }) }}
            </UiSelectValue>
        </UiSelectTrigger>
        <UiSelectContent>
            <UiSelectItem v-for="item in versions" :key="item.key" :value="item.key">
                {{ $t(`docs.versions.${item.key}`, { version: item.version }) }}
            </UiSelectItem>
        </UiSelectContent>
    </UiSelect>
</template>

<script setup lang="ts">
const props = defineProps<{ paths: Record<string, string> }>();
const emit = defineEmits<{ navigate: [] }>();

const { versions, version } = useDocsVersion();

const handleChange = (key: string) => {
    emit('navigate');
    navigateTo(props.paths[key]);
};
</script>
