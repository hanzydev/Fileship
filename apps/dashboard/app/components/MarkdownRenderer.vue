<template>
    <ClientOnly>
        <div
            ref="markdownContent"
            class="markdownContent p8"
            :data-variant="variant"
            v-html="renderMarkdown()"
        />
    </ClientOnly>
</template>

<script setup lang="ts">
import hljs from 'highlight.js';
import { Marked } from 'marked';
import { markedHighlight } from 'marked-highlight';
import markedKatex from 'marked-katex-extension';
import { render } from 'vue';

import { Icon, UiButton, UiTable } from '#components';

const {
    content,
    variant = 'primary',
    injectCopyButton = true,
} = defineProps<{
    content: string;
    variant?: 'primary' | 'secondary';
    injectCopyButton?: boolean;
}>();

const { $toast, vueApp } = useNuxtApp();

const markdownContentRef = useTemplateRef('markdownContent');

const marked = new Marked(
    markedHighlight({
        emptyLangClass: 'hljs',
        langPrefix: `${variant === 'primary' ? 'rounded-2xl' : 'rounded-xl'} hljs language-`,
        highlight(code, lang) {
            const language = hljs.getLanguage(lang) ? lang : 'plaintext';
            return hljs.highlight(code, { language }).value;
        },
    }),
);

marked.use({
    renderer: {
        link({ href, title, text }) {
            const iconVNode = h(Icon, {
                name: 'lucide:external-link',
                size: '16',
            });

            return `<a href="${href}" target="_blank" rel="noopener noreferrer" flex="~ items-center wrap gap-1" class="text-fs-accent hover:underline wfit"${title ? ` title="${title}"` : ''}><span>${text}</span>${vnodeToString(iconVNode)}</a>`;
        },
        table({ header, rows }) {
            const columnConfigs = header.map((cell) => ({
                key: cell.text,
            }));

            const formattedRows = rows.map((row) => {
                const rowObj: Record<string, string> = {};
                row.forEach((cell, index) => {
                    const columnKey = columnConfigs[index]!.key;
                    rowObj[columnKey] = cell.text;
                });
                return rowObj;
            });

            const tableVNode = h(UiTable, {
                columns: columnConfigs,
                rows: formattedRows,
                class: 'my-3 hfit!',
            });

            return vnodeToString(tableVNode);
        },
    },
});

marked.use(
    markedKatex({
        throwOnError: false,
    }),
);

const renderMarkdown = () => {
    const html = marked.parse(content, {
        breaks: true,
        gfm: true,
    });

    if (injectCopyButton) {
        nextTick(() => {
            const codeBlocks = markdownContentRef.value!.getElementsByClassName('hljs');
            for (const codeBlock of codeBlocks) {
                const pre = codeBlock.parentElement!;

                pre.classList.add('relative');

                const { copy, copied } = useClipboard({ legacy: true });

                const doRender = () => {
                    const copyButtonVNode = h(UiButton, {
                        alignment: 'center',
                        variant: 'glass',
                        class: [
                            'absolute top-4 right-4 size-10 !p0',
                            copied.value ? 'text-green500!' : 'text-fs-muted-2',
                            variant === 'primary' && 'rounded-xl!',
                        ],
                        icon: copied.value ? 'solar:clipboard-check-bold' : 'solar:clipboard-bold',
                        iconSize: '24',
                        'aria-label': 'Copy code to clipboard',
                        onClick: () => {
                            copy(codeBlock.textContent!);
                            $toast.success('Code copied to clipboard');
                        },
                    });

                    copyButtonVNode.appContext = vueApp._context;
                    render(copyButtonVNode, pre);
                };

                doRender();
                watch(copied, doRender);
            }
        });
    }

    return html;
};
</script>

<style scoped>
.markdownContent :deep(em) {
    @apply italic;
}

.markdownContent :deep(p code) {
    @apply px-2 py-1.5 font-mono text-sm;
}

.markdownContent :deep(p:has(code)) {
    @apply my-3;
}

.markdownContent :deep(ul) {
    @apply ml-3.5 list-outside list-disc;
}

.markdownContent :deep(ul ul) {
    @apply ml-3.5 list-circle;
}

.markdownContent :deep(ol) {
    @apply my-1 list-inside list-decimal;
}

.markdownContent :deep(h1),
.markdownContent :deep(h2),
.markdownContent :deep(h3) {
    @apply mb-3! mt-3! font-bold!;
}

.markdownContent :deep(> *:first-child) {
    @apply mt-0!;
}

.markdownContent :deep(hr) {
    @apply border-none! h-px my4;
}

.markdownContent :deep(blockquote) {
    @apply border-l-4 border-fs-accent pl-2.5 my-3 py-1.5 pr-1.5;
}

.markdownContent :deep(blockquote p) {
    @apply break-all;
}

.markdownContent[data-variant='primary'] :deep(p code) {
    @apply bg-fs-overlay-3 rounded-2xl;
}
.markdownContent[data-variant='primary'] :deep(hr) {
    @apply bg-fs-overlay-3;
}
.markdownContent[data-variant='primary'] :deep(blockquote) {
    @apply bg-fs-overlay-3 rounded-r-2xl;
}

.markdownContent[data-variant='primary'] :deep(> *:nth-child(-n + 2)) {
    @apply max-w-[calc(100%-140px)] break-all;
}

.markdownContent[data-variant='secondary'] :deep(p code) {
    @apply bg-fs-overlay-4 rounded-xl;
}
.markdownContent[data-variant='secondary'] :deep(hr) {
    @apply bg-fs-overlay-4;
}
.markdownContent[data-variant='secondary'] :deep(blockquote) {
    @apply bg-fs-overlay-4 rounded-r-xl;
}
</style>
