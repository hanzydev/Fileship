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
        <HighlightedCode
            v-else-if="preview && isHighlighted"
            :code="content"
            :language="language"
            line-numbers
        />
        <UiCodeBlock v-else>
            <UiCodeBlockContent wrap class="text-[0.85em]">{{ content }}</UiCodeBlockContent>
        </UiCodeBlock>
    </div>
</template>

<script setup lang="ts">
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

const isHighlighted = computed(
    () =>
        content.value!.length <= HIGHLIGHT_MAX_LENGTH &&
        !['plaintext', 'markdown'].includes(language.value),
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
