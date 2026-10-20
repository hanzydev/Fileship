<template>
    <div class="markdown" @click="handleClick" v-html="html" />
</template>

<script setup lang="ts">
import hljs from 'highlight.js';
import { Marked } from 'marked';
import markedKatex from 'marked-katex-extension';

const { content } = defineProps<{
    content: string;
}>();

const { $toast } = useNuxtApp();
const { t } = useI18n();
const { copy } = useClipboard({ legacy: true });

const copyIcon =
    '<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect width="14" height="14" x="8" y="8" rx="2"/><path d="M4 16c-1.1 0-2-.9-2-2V4c0-1.1.9-2 2-2h10c1.1 0 2 .9 2 2"/></svg>';

const escapeHtml = (value: string) =>
    value.replace(/[&<>"']/g, (char) => `&#${char.charCodeAt(0)};`);

const SAFE_PROTOCOLS = ['http:', 'https:', 'mailto:'];

const isSafeUrl = (url: string) => {
    try {
        return SAFE_PROTOCOLS.includes(new URL(url, 'https://relative.invalid').protocol);
    } catch {
        return false;
    }
};

const marked = new Marked(markedKatex({ throwOnError: false }), {
    renderer: {
        html({ text }) {
            return escapeHtml(text);
        },
        link({ href, title, text }) {
            if (!isSafeUrl(href)) return escapeHtml(text);

            return `<a href="${escapeHtml(href)}" target="_blank" rel="noopener noreferrer"${title ? ` title="${escapeHtml(title)}"` : ''}>${escapeHtml(text)}</a>`;
        },
        image({ href, title, text }) {
            if (!isSafeUrl(href)) return escapeHtml(text);

            return `<img src="${escapeHtml(href)}" alt="${escapeHtml(text)}"${title ? ` title="${escapeHtml(title)}"` : ''}>`;
        },
        code({ text, lang }) {
            const language = hljs.getLanguage(lang || '') ? lang! : 'plaintext';
            const highlighted = hljs.highlight(text, { language }).value;

            return `<pre><button type="button" data-copy aria-label="${escapeHtml(t('view.copyCode'))}">${copyIcon}</button><code class="hljs language-${language}">${highlighted}</code></pre>`;
        },
    },
});

const html = computed(() => marked.parse(content, { breaks: true, gfm: true }) as string);

const handleClick = (event: MouseEvent) => {
    const button = (event.target as HTMLElement).closest('[data-copy]');
    if (!button) return;

    copy(button.nextElementSibling!.textContent!);
    $toast.success(t('view.codeCopied'));
};
</script>

<style scoped>
@reference '~/styles/tailwind.css';

.markdown {
    @apply text-sm leading-7 break-words;
}

.markdown :deep(> * + *) {
    @apply mt-4;
}

.markdown :deep(h1) {
    @apply text-2xl font-bold tracking-tight;
}

.markdown :deep(h2) {
    @apply border-b pb-1 text-xl font-semibold tracking-tight;
}

.markdown :deep(h3) {
    @apply text-lg font-semibold;
}

.markdown :deep(a) {
    @apply font-medium text-primary underline underline-offset-4;
}

.markdown :deep(ul) {
    @apply ml-6 list-disc;
}

.markdown :deep(ol) {
    @apply ml-6 list-decimal;
}

.markdown :deep(blockquote) {
    @apply border-l-2 border-primary pl-4 text-muted-foreground italic;
}

.markdown :deep(hr) {
    @apply border-border;
}

.markdown :deep(:not(pre) > code) {
    @apply rounded-md bg-muted px-1.5 py-0.5 font-mono text-[0.85em];
}

.markdown :deep(pre) {
    @apply relative overflow-x-auto rounded-2xl;
}

.markdown :deep(pre code) {
    @apply block p-4 font-mono text-[0.85em];
}

.markdown :deep(pre [data-copy]) {
    @apply absolute top-2 right-2 flex size-8 items-center justify-center rounded-full bg-background/70 text-muted-foreground opacity-0 transition-opacity pointer-coarse:opacity-100 hover:text-foreground;
}

.markdown :deep(pre:hover [data-copy]) {
    @apply opacity-100;
}

.markdown :deep(table) {
    @apply w-full border-collapse text-left;
}

.markdown :deep(th),
.markdown :deep(td) {
    @apply border-b px-3 py-2;
}

.markdown :deep(th) {
    @apply font-medium text-muted-foreground;
}

.markdown :deep(img) {
    @apply max-w-full rounded-xl;
}
</style>
