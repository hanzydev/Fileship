<template>
    <div class="markdown">
        <template v-for="(segment, index) in segments" :key="index">
            <HighlightedCode
                v-if="segment.type === 'code'"
                :code="segment.code"
                :language="segment.language"
                :title="segment.language || 'text'"
            />
            <div v-else class="contents" v-html="segment.html" />
        </template>
    </div>
</template>

<script setup lang="ts">
import type { Token, TokensList } from 'marked';
import { Marked } from 'marked';
import markedKatex from 'marked-katex-extension';

const { content } = defineProps<{
    content: string;
}>();

type Segment = { type: 'code'; code: string; language: string } | { type: 'html'; html: string };

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
    breaks: true,
    gfm: true,
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
        code({ text }) {
            return `<pre><code>${escapeHtml(text)}</code></pre>`;
        },
    },
});

const segments = computed(() => {
    const tokens = marked.lexer(content);
    const result: Segment[] = [];
    let run: Token[] = [];

    const flush = () => {
        if (!run.length) return;
        result.push({
            type: 'html',
            html: marked.parser(Object.assign(run, { links: tokens.links }) as TokensList),
        });
        run = [];
    };

    for (const token of tokens) {
        if (token.type === 'code') {
            flush();
            result.push({ type: 'code', code: token.text, language: token.lang ?? '' });
        } else if (token.type !== 'space') {
            run.push(token);
        }
    }

    flush();

    return result;
});
</script>

<style scoped>
@reference '~/styles/tailwind.css';

.markdown {
    @apply flex flex-col gap-4 text-sm leading-7 break-words;
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

.markdown :deep(pre:not([data-slot])) {
    @apply overflow-x-auto rounded-2xl bg-muted p-4 font-mono text-[0.85em];
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
