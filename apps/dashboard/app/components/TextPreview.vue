<template>
    <div>
        <EmptyState
            v-if="isTooLarge"
            :icon="IFileText"
            :title="$t('textPreview.tooLarge')"
            :description="
                $t('textPreview.tooLargeDescription', {
                    size: formatStorageSize(TEXT_PREVIEW_MAX_SIZE, locale),
                })
            "
        >
            <slot name="too-large" />
        </EmptyState>
        <TextBlock v-else-if="error" variant="muted">{{ error }}</TextBlock>
        <div v-else-if="content === undefined" class="flex flex-col gap-3">
            <TextBlock v-for="i in 6" :key="i" custom-tag="div" variant="muted" class="w-full">
                <UiSkeleton
                    class="h-lh rounded-full"
                    :style="{ width: `${90 - ((i * 13) % 40)}%` }"
                />
            </TextBlock>
        </div>
        <MarkdownRenderer v-else-if="preview && language === 'markdown'" :content="content" />
        <pre
            v-else-if="preview && highlighted"
            class="overflow-x-auto rounded-2xl"
        ><code class="hljs block p-4 font-mono text-[0.85em]" v-html="highlighted" /></pre>
        <pre v-else class="font-mono text-sm break-all whitespace-pre-wrap">{{ content }}</pre>
    </div>
</template>

<script setup lang="ts">
import hljs from 'highlight.js';

import { IFileText } from '#components';

const HIGHLIGHT_MAX_LENGTH = 512 * 1024;

const {
    src,
    name,
    mimeType,
    size,
    preview = true,
} = defineProps<{
    src: string;
    name: string;
    mimeType: string;
    size: number;
    preview?: boolean;
}>();

const content = defineModel<string>('content');

const { locale } = useI18n();

const error = ref<string>();

const language = computed(() => textPreviewLanguage(name, mimeType) ?? 'plaintext');
const isTooLarge = computed(() => size > TEXT_PREVIEW_MAX_SIZE);

const highlighted = computed(() =>
    content.value &&
    content.value.length <= HIGHLIGHT_MAX_LENGTH &&
    !['plaintext', 'markdown'].includes(language.value) &&
    hljs.getLanguage(language.value)
        ? hljs.highlight(content.value, { language: language.value, ignoreIllegals: true }).value
        : null,
);

let version = 0;

const load = async () => {
    const current = ++version;

    content.value = undefined;
    error.value = undefined;

    if (isTooLarge.value) return;

    try {
        const text = await $fetch<string>(src, { responseType: 'text' });
        if (current === version) content.value = text;
    } catch (fetchError) {
        if (current === version) error.value = getErrorMessage(fetchError);
    }
};

watch(() => src, load);

onMounted(load);
</script>
