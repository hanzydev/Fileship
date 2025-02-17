<template>
    <div class="codeBlock">
        <div :class="language === 'markdown' && !fullScreen && 'space-y-4'">
            <div
                v-if="language === 'markdown'"
                flex="~ gap2 items-center"
                h16
                p4
                :class="[
                    !fullScreen && 'rounded',
                    shouldRenderMarkdown && fullScreen ? 'bg-fs-overlay-1' : 'bg-fs-overlay-3',
                ]"
            >
                <Icon name="heroicons-solid:information-circle" size="20" text-fs-accent />
                <p text-slate300 text-sm="!">
                    {{
                        renderMarkdown
                            ? "You're viewing the rendered markdown"
                            : "You're viewing the raw markdown"
                    }}
                </p>

                <UiButton
                    :class="
                        fullScreen &&
                        shouldRenderMarkdown &&
                        '!bg-fs-overlay-1 hover:!bg-fs-overlay-2'
                    "
                    mla
                    h8
                    w8
                    p0="!"
                    alignment="center"
                    :variant="fullScreen && shouldRenderMarkdown ? 'primary' : 'secondary'"
                    :icon="renderMarkdown ? 'heroicons-solid:code' : 'heroicons-solid:eye'"
                    icon-size="20"
                    aria-label="Toggle markdown view"
                    @click="renderMarkdown = !renderMarkdown"
                />
            </div>

            <div relative>
                <pre
                    v-if="!shouldRenderMarkdown"
                ><code rounded :class="[`hljs language-${language}`, fullScreen ? language === 'markdown' ? 'min-h-[calc(100vh-4rem)]' : 'min-hscreen' : '']" v-html="html" /></pre>
                <div
                    v-else
                    bg-fs-overlay-3
                    p4
                    :class="
                        fullScreen
                            ? language === 'markdown'
                                ? 'min-h-[calc(100vh-4rem)]'
                                : 'min-hscreen'
                            : 'rounded'
                    "
                    v-html="html"
                />

                <UiButton
                    v-if="!shouldRenderMarkdown"
                    absolute
                    right-4
                    top-4
                    h8
                    w8
                    bg-transparent
                    p0="!"
                    :class="[' hover:bg-white/5', copied && 'text-green500']"
                    alignment="center"
                    :icon="
                        copied
                            ? 'heroicons-solid:clipboard-check'
                            : 'heroicons-solid:clipboard-copy'
                    "
                    icon-size="20"
                    aria-label="Copy code"
                    @click="handleCopy"
                />
            </div>
        </div>
    </div>
</template>

<script setup lang="ts">
import hljs from 'highlight.js';
import { Marked } from 'marked';
import { markedHighlight } from 'marked-highlight';
import { toast } from 'vue-sonner';

const { language, code } = defineProps<{
    language: string;
    code: string;
    fullScreen?: boolean;
}>();

const renderMarkdown = ref(true);

const shouldRenderMarkdown = computed(() => language === 'markdown' && renderMarkdown.value);

const { copied, copy } = useClipboard({ legacy: true });

const marked = new Marked(
    markedHighlight({
        langPrefix: 'rounded hljs language-',
        highlight(code, lang) {
            const language = hljs.getLanguage(lang) ? lang : 'plaintext';
            return hljs.highlight(code, { language }).value;
        },
    }),
);

const html = computed(() => {
    registerLanguage();

    return shouldRenderMarkdown.value
        ? marked.parse(code)
        : hljs.highlight(code, { language }).value;
});

const registerLanguage = () => {
    import(/* @vite-ignore */ `highlight.js/lib/languages/${language}`)
        .then((module) => {
            hljs.registerLanguage(language, module.default);
        })
        .catch(() => null);
};

const handleCopy = () => {
    copy(code);
    toast.success('Code copied to clipboard');
};
</script>

<style>
.codeBlock h1:not(:first-child),
.codeBlock h2,
.codeBlock h3,
.codeBlock h4,
.codeBlock h5,
.codeBlock h6,
.codeBlock blockquote,
.codeBlock ul,
.codeBlock ol,
.codeBlock dl,
.codeBlock table,
.codeBlock pre:is(pre, code):not(:first-child) {
    @apply mt4;
}

.codeBlock table {
    @apply wfull;
}

.codeBlock table th,
.codeBlock table td {
    @apply px4 py2;
}

.codeBlock table th {
    @apply bg-fs-overlay-4 first:rounded-l-sm last:rounded-r-sm;
}
</style>
