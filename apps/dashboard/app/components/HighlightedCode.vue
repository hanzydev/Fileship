<template>
    <UiCodeBlock>
        <UiCodeBlockHeader v-if="title">
            <UiCodeBlockTitle class="font-mono text-xs">{{ title }}</UiCodeBlockTitle>
            <UiCodeBlockActions>
                <CodeCopyButton :value="code" />
            </UiCodeBlockActions>
        </UiCodeBlockHeader>
        <UiCodeBlockContent :line-numbers="lineNumbers" class="text-[0.85em]">
            <UiCodeBlockLine v-for="(line, index) in lines" :key="index">
                <UiCodeBlockToken v-for="(token, i) in line" :key="i" :token="token" />
            </UiCodeBlockLine>
        </UiCodeBlockContent>
    </UiCodeBlock>
</template>

<script setup lang="ts">
import { useHighlightedCode } from '#layers/ui/app/components/ui/code-block';

const { code, language } = defineProps<{
    code: string;
    language: string;
    title?: string;
    lineNumbers?: boolean;
}>();

const lines = useHighlightedCode(
    () => code,
    () => language,
);
</script>
