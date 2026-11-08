<template>
    <UiCodeBlock class="my-6">
        <UiCodeBlockHeader>
            <UiCodeBlockTitle class="font-mono text-xs">{{ label }}</UiCodeBlockTitle>
            <UiCodeBlockActions>
                <CodeCopyButton :value="code ?? ''" />
            </UiCodeBlockActions>
        </UiCodeBlockHeader>
        <UiCodeBlockContent :class="cn('text-[13px]', $props.class)" :style="style">
            <slot />
        </UiCodeBlockContent>
    </UiCodeBlock>
</template>

<script setup lang="ts">
const props = defineProps<{
    code?: string;
    language?: string | null;
    filename?: string | null;
    class?: string | null;
    style?: string | Record<string, string> | null;
}>();

const label = computed(() => props.filename || props.language || 'text');
</script>
